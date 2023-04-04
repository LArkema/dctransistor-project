/*
 * Google Apps Script (JavaScript) code to handle DCTransistor orders
 * File is a copy of test and development code as of 4/3/2023, and may not reflect the actual state of code in production as business logic is modified or bugs are fixed.
 * (c) Logan Arkema, 2023
 * MIT License
*/



const STRIPE_AUTH_HEADER = 'Bearer <Token>';
const STRIPE_API_BASE = 'https://api.stripe.com/v1/';
const STRIPE_NOTIFICATIONS_EMAIL = "notifications@stripe.com";

const SHIPPO_AUTH_HEADER = 'ShippoToken <Token>';
const SHIPPO_API_BASE = 'https://api.goshippo.com/';
const SHIPPO_USPS_CARRIER_ACCOUNT = "<Carrier Account";
const SHIPPO_LABEL_TYPE = "PDF_4x6";

//constants that meet requirements for pickups.location object variables in Shippo API
// https://docs.goshippo.com/shippoapi/public-api/#tag/Pickups
const PICKUP_LOCATION_TYPE = "<building_location_type";
const PICKUP_BUILDING_TYPE = "<building_type>";
const PICKUP_INSTRUCTIONS = "<instructions";

const ORDERS_SPREADSHEET_ID = "<Google Sheets ID";
const TIMEZONE = 'America/New_York';
const SHIPPO_DT_FORMAT = "yyyy-MM-dd'T'HH:mm:ss.SSS'Z'";
const EMAIL_DT_FORMAT = "MMMM dd, yyyy";
const SPREADSHEET_DT_FORMAT = "MM/dd/yyyy";

//Object with variables necessary to complete address_from in Shipments and location.address in pickups in Shippo API
// https://docs.goshippo.com/shippoapi/public-api/#tag/Pickups
// https://docs.goshippo.com/shippoapi/public-api/#tag/Shipments
const MY_ADDRESS = {
 <JS OBJECT WITH SHIPPING ADDRESS DETAILS
}

//When email arrives, check if payment notification from Stripe then begin order processing.
function parseEmailForPaymentId(){

  //Get all emails from Stripe with notification of payment
  //const search_string = "is:unread from:"+STRIPE_NOTIFICATIONS_EMAIL+""
  var payment_notifications = GmailApp.search('is:unread from:'+STRIPE_NOTIFICATIONS_EMAIL+' subject:Payment');

  if(payment_notifications.length == 0){return;}

  for (i=0; i<payment_notifications.length; i++){
    var message = GmailApp.getMessagesForThread(payment_notifications[i])[0];
    var html = message.getRawContent();
    
    var payment_intent_regex = new RegExp('^pi_[A-Za-z0-9]*', 'mg');
    var regex_matches = html.match(payment_intent_regex);
    var payment_intent = regex_matches[0];

    var order_date = message.getDate();
    message.markRead();

    //Get charge number associated with payment intent identified in email notification
    var options = {
      'headers': {
        'Authorization': STRIPE_AUTH_HEADER
      },
    };
    var req_url = STRIPE_API_BASE + 'payment_intents/' + payment_intent;
    var response = UrlFetchApp.fetch(req_url, options);

    //Get full charge data and send for parsing / processing.
    var charge = JSON.parse(response).latest_charge;
    req_url = STRIPE_API_BASE + 'charges/' + charge;
    response = UrlFetchApp.fetch(req_url, options);

    //Get all relevant payment data from Stripe charge object
    getStripePaymentData(JSON.parse(response), order_date);
  }//end loop through payment notification emails

}//END parseEmailForPaymentId

//helper function to get column index by header value
function getColumnByName(sheet, name){
  return sheet.getDataRange().getValues()[0].indexOf(name);
}

function createShippoLabel(shippoData, order_row){

  //Turn full shipping method title to Shippo token
  var shippo_servicelevel = ''
  switch(shippoData.shipping_method){
    case 'USPS Priority':
      shippo_servicelevel = 'usps_priority';
      break;
    case 'USPS First Class':
      shippo_servicelevel = 'usps_first';
      break;
    case 'USPS Parcel Select':
      shippo_servicelevel = 'usps_parcel_select';
      break;
    default:
      shippo_servicelevel = '';
      console.error("Invalid shipping method");
  } //end getting shippo service token

  //Get Spreadsheet Columns indexes for Orders
  var orders_sheet = SpreadsheetApp.openById(ORDERS_SPREADSHEET_ID).getSheetByName('Orders');
  var label_url_column = getColumnByName(orders_sheet,"Shipping Label URL");
  var label_transaction_column = getColumnByName(orders_sheet,"Shipping Label Transaction ID");
  var usps_tracking_column = getColumnByName(orders_sheet, "USPS Tracking URL");
  var pickup_status_column = getColumnByName(orders_sheet,"Pickup Scheduled");

  //Recipient, sender, and package information necessary to generate USPS label
  var label_data = {
    'shipment' : {
      'address_from' : {
        'name': MY_ADDRESS.name,
        'company': MY_ADDRESS.company,
        'street1': MY_ADDRESS.street_1,
        'street2': MY_ADDRESS.street_2,
        'city': MY_ADDRESS.city,
        'state': MY_ADDRESS.state,
        'zip': MY_ADDRESS.zip,
        'country': MY_ADDRESS.country,
        'phone': MY_ADDRESS.phone,
        'email': MY_ADDRESS.email,
        'is_residential': MY_ADDRESS.residential
      },
      'address_to': {
        'name': shippoData.name,
        'street1': shippoData.street_1,
        'street2': shippoData.street_2,
        'city': shippoData.city,
        'state': shippoData.state,
        'zip': shippoData.zip,
        'country': shippoData.country,
        'email': shippoData.shippo_email 
      },
      'parcels':[{
        'distance_unit': "in",
        'height': "0.25",
        'length': "12",
        'width': "12",
        'mass_unit': "oz",
        'weight': "7"
      }]
    },
    'carrier_account': SHIPPO_USPS_CARRIER_ACCOUNT,
    'servicelevel_token': shippo_servicelevel,
    'label_file_type': SHIPPO_LABEL_TYPE
  };

  //HTTP headers and protocol options
  var options = {
    'headers': {
      'Authorization': SHIPPO_AUTH_HEADER,
    },
    'contentType': 'application/json',
    'method': 'post',
    'payload': JSON.stringify(label_data),
    'muteHttpExceptions': true
  };

  //Send shippment and label generation transaction to Shippo
  var req_url = SHIPPO_API_BASE + 'transactions/';
  var response = UrlFetchApp.fetch(req_url, options);

  var response_data = JSON.parse(response);

  //Get relevant data from shipping label response
  var label_url = response_data.label_url;
  var transaction_id = response_data.object_id;
  var tracking_number = response_data.tracking_number;
  var usps_tracking_url = response_data.tracking_url_provider;
  var transaction_status = response_data.status;

  //Fill email template with order information to send when label created.
  let order_template = HtmlService.createTemplateFromFile('ShippingEmail');
  order_template.name = shippoData.name;
  order_template.receipt_num = shippoData.receipt_number;
  order_template.receipt_url = shippoData.receipt_url;
  order_template.usps_shipment_link = usps_tracking_url;
  order_template.usps_shipment_num = tracking_number;
  let email_body = order_template.evaluate().getContent();
  var email_subject = "Confirmation: DCTransistor Board Order " + shippoData.receipt_number;

  //If label creation is a success, update orders spreadsheet and send email to customer.
  if (transaction_status == "SUCCESS"){
    var order_range = orders_sheet.getRange(order_row, 1, 1, 50);
    order_range.getCell(1,label_url_column+1).setValue(label_url);
    order_range.getCell(1, label_transaction_column+1).setValue(transaction_id);
    order_range.getCell(1, usps_tracking_column+1).setValue(usps_tracking_url);
    order_range.getCell(1, pickup_status_column+1).setValue("False");
    sendCustomerEmail(shippoData.email, email_subject, email_body);
  }
  else {
    var order_range = orders_sheet.getRange(order_row, 1, 1, 50);
    order_range.getCell(1,label_url_column+1).setValue("ERROR");
    console.error(response_data);
  }

}//END createShippoLabel

//Helper Function to send customers updates from orders@dctransistor.com
function sendCustomerEmail(recepient_address, subject, email_body){
   GmailApp.sendEmail(recepient_address, subject, email_body, {
     htmlBody: email_body, 
     name: "DCTransistor Orders",
     from: "orders@DCTransistor.com",
     replyTo: "orders@DCTransistor.com"
   });
}//END sendCustomerEmail

//Manually triggered function to create a pickup for orders awaiting pickup
function createUspsPickup(){

  //Get Date-Time String to send to USPS for acceptable pickup range (tomorrow to three days from now)
    var tomorrow = new Date();
    tomorrow.setDate(tomorrow.getDate() + 1);
    tomorrow.setHours(7);
    tomorrow.setMinutes(0);

    var three_days_from_now  = new Date();
    three_days_from_now.setDate(three_days_from_now.getDate() + 3);
    three_days_from_now.setHours(23);
    three_days_from_now.setMinutes(0);

    var pickup_date_earliest = Utilities.formatDate(tomorrow, TIMEZONE, SHIPPO_DT_FORMAT);
    var pickup_date_latest = Utilities.formatDate(three_days_from_now, TIMEZONE, SHIPPO_DT_FORMAT);
  //end setting pickup Date-Times

  //Get pickup status data and info needed to send update email from Orders spreadsheet
    var orders_sheet = SpreadsheetApp.openById(ORDERS_SPREADSHEET_ID).getSheetByName('Orders');
    var orders_data = orders_sheet.getDataRange().getValues();

    const name_column = getColumnByName(orders_sheet, "Name");
    const email_column = getColumnByName(orders_sheet, "Email");
    const receipt_num_column = getColumnByName(orders_sheet, "Receipt Number");
    const usps_tracking_column = getColumnByName(orders_sheet, "USPS Tracking URL");
    const pickup_status_column = getColumnByName(orders_sheet,"Pickup Scheduled");
    const label_transaction_column = getColumnByName(orders_sheet,"Shipping Label Transaction ID");
    const pickup_confirmation_column = getColumnByName(orders_sheet,"Pickup Confirmation Code");
    const ship_date_column = getColumnByName(orders_sheet,"Ship Date");
  //end getting spreadsheet column data

  //Get row indexes for orders needing pickup
  var orders_needing_pickup = [];
  for (i = 0; i < orders_sheet.getLastRow(); i++){
    var tmp = orders_data[i][pickup_status_column];
    if (orders_data[i][pickup_status_column] === false){
      orders_needing_pickup.push(i);
    }
  }

  //Get transaction IDs for orders needing pickup
  var orders_label_transaction_ids = [];
  for (i = 0; i<orders_needing_pickup.length; i++){
    orders_label_transaction_ids.push(orders_data[orders_needing_pickup[i]][label_transaction_column]);
  }

  //Shippo API form data needed to schedule pickup
  var pickup_data = {
    'carrier_account': SHIPPO_USPS_CARRIER_ACCOUNT,
    'location' : {
      'address' : {
        'name': MY_ADDRESS.name,
        'company': MY_ADDRESS.company,
        'street1': MY_ADDRESS.street_1,
        'street2': MY_ADDRESS.street_2,
        'city': MY_ADDRESS.city,
        'state': MY_ADDRESS.state,
        'zip': MY_ADDRESS.zip,
        'country': MY_ADDRESS.country,
        'phone': MY_ADDRESS.phone,
        'email': MY_ADDRESS.email,
        'is_residential': MY_ADDRESS.residential          
      },
      'building_location_type': PICKUP_LOCATION_TYPE,
      'building_type': PICKUP_BUILDING_TYPE,
      'instructions': PICKUP_INSTRUCTIONS
    },
    'transactions' : orders_label_transaction_ids,
    'requested_start_time' : pickup_date_earliest,
    'requested_end_time' : pickup_date_latest,
    'is_test': false
  }//end pickup_data

  //HTTP Authorization Header and Protocol Options for POST request
  var options = {
    'headers': {
      'Authorization': SHIPPO_AUTH_HEADER,
    },
    'contentType': 'application/json',
    'method': 'post',
    'payload': JSON.stringify(pickup_data),
    'muteHttpExceptions': true
  };

  //Create USPS Pickup through Shippo
  var req_url = SHIPPO_API_BASE + 'pickups/';
  var response = UrlFetchApp.fetch(req_url, options);
  var response_data = JSON.parse(response);

  const pickup_confirmation_code = response_data.confirmation_code;
  const pickup_date = new Date(response_data.confirmed_end_time);

  //Create variable for pickup template
  let pickup_template = HtmlService.createTemplateFromFile('PickupEmail');

  //If pickup schedule is successful, change tracking status in spreadsheet and sent appropriate emails.
  if (response_data.status == "CONFIRMED"){
    for (i = 0; i< orders_needing_pickup.length; i++){
      //For each order needing pickup in spreadsheet, update data with pickup details
      var order_row = orders_sheet.getRange(orders_needing_pickup[i] + 1, 1, 1, 50);
      var order_values = order_row.getValues();
      order_row.getCell(1, pickup_status_column+1).setValue("TRUE");
      order_row.getCell(1, pickup_confirmation_column+1).setValue(pickup_confirmation_code);
      order_row.getCell(1, ship_date_column+1).setValue(Utilities.formatDate(pickup_date, TIMEZONE, SPREADSHEET_DT_FORMAT));

      //Retrieve additional data necessary to send pickup confirmation email
      pickup_template.name = order_values[0][name_column];
      pickup_template.receipt_num = order_values[0][receipt_num_column];
      pickup_template.usps_shipment_link = order_values[0][usps_tracking_column];
      pickup_template.pickup_date = Utilities.formatDate(pickup_date, TIMEZONE, EMAIL_DT_FORMAT);
      var email_address = order_values[0][email_column];

      //Send pickup confirmation email
      let email_body = pickup_template.evaluate().getContent();
      let email_subject = "DCTransistor Shipment Scheduled";
      sendCustomerEmail(email_address, email_subject, email_body);
    }//end loop through all orders awaiting pickup
  }//end on confirmed pickup

  //If not confirmed pickup, make note in spreadsheet and log error (likely pickup already scheduled).
  else {
    var order_row = orders_sheet.getRange(orders_needing_pickup[0], 1, 1, 50);
    order_row.getCell(1, pickup_status_column+1).setValue("ERROR: View Execution Logs");
    Logger.log(response_data);  
    console.error(response_data);
  }

}//end createUspsPickup

//Helper function to strip away HTML from single value in Stripe receipt
function parseReceiptHTML(value){
  if (!value.includes("<strong>")){
    var substring = value.match(/\n\s+[$\w\d].*/);
    var result = substring[0].match(/[^\s$].*/)[0];
  }
  else{
    var substring = value.match(/<strong>.*</);
    var tmp = substring[0].match(/>.*</);
    var result = tmp[0].match(/[^<>:$]+/)[0];
  }
  return result;
}

//Overall function to get key order information from HTML stripe receipt
function parseStripeReceipt(receipt){

  receipt.replace('\\n/g', '\n');


  var description_regex = new RegExp('class="Table-description.*\n.*','mg');
  var value_regex = new RegExp('class="Table-amount.*\n.*','mg');
  var email_sharing_regex = new RegExp('Share email with USPS for shipping updates:</strong>.*');

  //Get value of consent to share email address with shipping providers
  var consent_match = receipt.match(email_sharing_regex);
  var consent_result_tmp = consent_match[0].match(/>.*/);
  var share_email_with_shippers = consent_result_tmp[0].match(/\w+/)[0];


  var description_matches = receipt.match(description_regex);
  var value_matches = receipt.match(value_regex);

  var product;
  var subtotal;
  var shipping;
  var amount_charged;
  var shipping_method;

  //Loop through all HTML elements with descriptions and values associated with them
  for (var i=0; i < value_matches.length; i++){

    //Get description for line, then get value for associated line.
    line = parseReceiptHTML(description_matches[i]);

    if(line.includes("DCTransistor")){
      product = line;
    }

    //Parse subtotal
    if(line == "Subtotal"){
      subtotal = Number(parseReceiptHTML(value_matches[i]));
    }

    //parse shipping method and cost
    else if(line.includes("Shipping")){
      shipping = Number(parseReceiptHTML(value_matches[i]));

      if(line.includes("USPS Priority Mail")){
        shipping_method = "USPS Priority";
      }
      else{
        shipping_method = "Local Pickup";
      }
    }

    //Parse total amount charged
    else if(line == "Amount charged"){
      amount_charged = Number(parseReceiptHTML(value_matches[i]));
    }
  }// end HTML loop

  //return all key values as single object
  return{
    product: product,
    subtotal: subtotal,
    shipping: shipping,
    amount_charged: amount_charged,
    shipping_method: shipping_method,
    share_email_consent: share_email_with_shippers,
  }

}//END parseStripeReceipt


//Main function to get all key data from a Stripe payment, including parsing HTML receipt with payment details
function getStripePaymentData(payment, order_date){

  //Get column indexes from order tracking spreadsheet
    var orders_sheet = SpreadsheetApp.openById(ORDERS_SPREADSHEET_ID).getSheetByName('Orders');
    const payment_id_column = getColumnByName(orders_sheet, "Payment ID");
    const receipt_num_column = getColumnByName(orders_sheet, "Receipt Number");
    const name_column = getColumnByName(orders_sheet, "Name");
    const email_column = getColumnByName(orders_sheet, "Email");
    const product_column = getColumnByName(orders_sheet, "Product");
    const total_column = getColumnByName(orders_sheet,"Total");
    const product_price_column = getColumnByName(orders_sheet,	"Product Price");
    const shipping_price_column = getColumnByName(orders_sheet,	"Shipping Price");
    const shipping_method_column = getColumnByName(orders_sheet, "Shipping Method");
    const zip_column = getColumnByName(orders_sheet, "Zip Code");
    const email_sharing_column = getColumnByName(orders_sheet, "Email Sharing Consent");
    const local_pickup_column = getColumnByName(orders_sheet, "Local Pickup Arranged");
    const order_date_column = getColumnByName(orders_sheet,"Order Date");
  //end getting column indexes

  //If payment did not create a sucessful purchase, exit function
  if(payment.amount_captured == false){
    return;
  }

  //Extract relevant order details from order receipt
  var receipt_url = payment.receipt_url;
  var receipt_out = UrlFetchApp.fetch(receipt_url).getContentText();
  const {product, subtotal, shipping, amount_charged, shipping_method, share_email_consent} = parseStripeReceipt(receipt_out);

  //Extract relevant details from payment API response
  var amount_captured = payment.amount_captured;
  var receipt_number = payment.receipt_number;
  var payment_id = payment.id;
  var name = payment.shipping.name;
  var email = payment.billing_details.email;

  //Log if billing details do not add up (will log issue for local pickup)
  if( (amount_charged*100) != amount_captured || (subtotal+shipping) != amount_charged){
    Logger.log("Payment Numbers Do Not Add Up");
  }

  //Log order details to spreadsheet
    var row_index = orders_sheet.getLastRow()+1;
    var order_row = orders_sheet.getRange(row_index, 1, 1, 50);
    order_row.getCell(1, payment_id_column+1).setValue(payment_id);
    order_row.getCell(1, receipt_num_column+1).setValue(receipt_number);
    order_row.getCell(1, email_column+1).setValue(email);
    order_row.getCell(1, product_column+1).setValue(product);
    order_row.getCell(1, total_column+1).setValue(amount_charged);
    order_row.getCell(1, product_price_column+1).setValue(subtotal);
    order_row.getCell(1, shipping_price_column+1).setValue(shipping);
    order_row.getCell(1, name_column+1).setValue(name);
    order_row.getCell(1, shipping_method_column+1).setValue(shipping_method);
    order_row.getCell(1, zip_column+1).setValue(payment.shipping.address.postal_code);
    order_row.getCell(1, email_sharing_column+1).setValue(share_email_consent);
    order_row.getCell(1, order_date_column+1).setValue(Utilities.formatDate(order_date, TIMEZONE, SPREADSHEET_DT_FORMAT));
  //end filling out spreadsheet

  //If creating USPS shipment, create shipping label and mark proper spreadsheet values
  if (shipping_method != "Local Pickup"){

    order_row.getCell(1, local_pickup_column+1).setValue("N/A");

    const address = payment.shipping.address;
    const addr_line_1 = address.line1;
    const addr_line_2 = address.line2;
    const addr_city = address.city;
    const addr_state = address.state;
    const addr_zip = address.postal_code;
    const addr_country = address.country;
    var shippo_email = '';

    if(share_email_consent == "Yes"){
      shippo_email = email;
    }

    var shippoData = {
      email: email,
      receipt_number: receipt_number,
      receipt_url: receipt_url,
      shippo_email: shippo_email,
      shipping_method: shipping_method,
      name: name,
      street_1: addr_line_1,
      street_2: addr_line_2,
      city: addr_city,
      state: addr_state,
      zip: addr_zip,
      country: addr_country,
    };

    createShippoLabel(shippoData, row_index);
  } //END  create Shippo Shipment logic

  //If local pickup, send self email to follow-up and set spreadsheet values
  else {
    order_row.getCell(1, email_sharing_column+1).setValue("N/A");
    order_row.getCell(1, local_pickup_column+1).setValue("FALSE");

    //Prepare email variables to send to customer
    let pickup_template = HtmlService.createTemplateFromFile('LocalPickupEmail');
    pickup_template.name = name;
    pickup_template.receipt_url = receipt_url;
    pickup_template.receipt_num = receipt_number;
    let email_body = pickup_template.evaluate().getContent();
    var email_subject = "Confirmation: DCTransistor Board Order " + receipt_number;

    //Send email to customer confirming order and to self alerting to new local pickup
    sendCustomerEmail("orders@dctransistor.com", "NEW LOCAL PICKUP", receipt_number);
    sendCustomerEmail(email, email_subject, email_body);
  }//end local pickup logic

}// END getStripePaymentData

//Function to run daily and check if shipped boards have been delivered.
function checkDelivery(){

 //Get orders column indecies to preserve
    var orders_sheet = SpreadsheetApp.openById(ORDERS_SPREADSHEET_ID).getSheetByName('Orders');
    const receive_date_column = getColumnByName(orders_sheet, "Receive Date");
    const transaction_id_column = getColumnByName(orders_sheet, "Shipping Label Transaction ID");
    var orders_values = orders_sheet.getDataRange().getValues();
  //end column indicies

  //HTTP Authorization Header and Protocol Options for POST request
  var options = {
    'headers': {
      'Authorization': SHIPPO_AUTH_HEADER,
    },
    'method': 'get',
  };

  for(i=1; i<orders_sheet.getLastRow(); i++){
    var transaction_id = orders_values[i][transaction_id_column];
    var receive_date = orders_values[i][receive_date_column];

    //If transaction ID exists (i.e. order not week since delivery) and receive date does not, check for delivery status
    if(transaction_id != '' && receive_date == ''){
      var req_url = SHIPPO_API_BASE + 'transactions/' + transaction_id;
      var response = UrlFetchApp.fetch(req_url, options);
      var response_data = JSON.parse(response);

      //If response data is delivered, update receive_date
      if(response_data.tracking_status == "DELIVERED"){
        orders_sheet.getRange(i+1, receive_date_column+1,1,1).getCell(1,1)
          .setValue(Utilities.formatDate(new Date(), TIMEZONE, SPREADSHEET_DT_FORMAT));
      }
    }
  }//end loop through all orders

}//END checkDelivery()

//Function that runs daily to delete data in spreadsheet
function deleteData(){

  //Get orders column indecies to preserve
    var orders_sheet = SpreadsheetApp.openById(ORDERS_SPREADSHEET_ID).getSheetByName('Orders');
    const product_column = getColumnByName(orders_sheet, "Product");
    const total_column = getColumnByName(orders_sheet,"Total");
    const product_price_column = getColumnByName(orders_sheet,	"Product Price");
    const shipping_price_column = getColumnByName(orders_sheet,	"Shipping Price");
    const shipping_method_column = getColumnByName(orders_sheet, "Shipping Method");
    const zip_column = getColumnByName(orders_sheet, "Zip Code");
    const order_date_column = getColumnByName(orders_sheet,"Order Date");
    const ship_date_column = getColumnByName(orders_sheet, "Ship Date")
    const receive_date_column = getColumnByName(orders_sheet, "Receive Date");
  //end column indicies

  var today = new Date();
  var preserve_columns = [product_column, total_column, product_price_column, shipping_price_column, shipping_method_column, zip_column, order_date_column, ship_date_column, receive_date_column];

  //loop through all rows and check for uncleared orders delivered a week or more ago
  for(i=2; i<=orders_sheet.getLastRow(); i++){

    var order_row = orders_sheet.getRange(i,1,1,50);
    var receive_date = new Date(order_row.getCell(1,receive_date_column+1).getValue());
    var compare_date = new Date(receive_date);
    compare_date.setDate(receive_date.getDate() + 7);

    //If a week since board received and data not already wiped, wipe data 
    if(today.getTime() >= compare_date.getTime() && order_row.getCell(1,1).getValue() != ''){

      //For every column, set to blank if its index is not stored in preserve columns
      for(j=1; j<50; j++){
        if(!preserve_columns.includes(j-1)){
          order_row.getCell(1,j).setValue('');
        }
      }
    }
  }//end loop through all rows

}//END deleteData

//Create menu and button to create a USPS pickup in Google Sheet Menu UI
function onOpen(){
  var ui = SpreadsheetApp.getUi();
  ui.createMenu('DCTransistor')
  .addItem('Create USPS Pickup', 'createUspsPickup')
  .addToUi();
}

