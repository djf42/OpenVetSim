<?php
/*
sim-player: 

Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>
*/


	class mailClass {
		function __construct() {
		}
		
		static public function mailTest($message) {
			$messageHTML = '
				<!DOCTYPE html>
				<html>
					<head>
						<title>Test Email</title>
					</head>

					<body>
						<p>I think I fixed it!!!</p>
					</body>
				</html> 
			';

			// Set the message content
			$message = new Mail_mime("\n");
			$message->setTXTBody($messageText);
			$message->setHTMLBody($messageHTML);

			// Get the body and headers
			$body = $message->get();
			$headers = $message->headers(array(
			'From' => 'dweiner@twcny.rr.com',
			'Subject' => 'Test Email',
			));

			// Send the message
			$mail = @Mail::factory('mail');
			//	echo 'Status: '.$mail->send('admin@paperdollspaperie.com', $headers, $body);
			$mail->send('david@nextinteractives.com', $headers, $body);
		}
		
		static public function createEmailReceipt($orderID, $extraMessage = '') {
			// validate order
			$orderRow = cartClass::getOrderRow($orderID);
			if($orderRow === FALSE) {
				exit();
			}
//FB::log($orderRow);		
			// validate order status
			$orderStatusID = $orderRow['OrderOrderStatusID'];
			$orderStatusRow = cartClass::getOrderStatusRow($orderStatusID);
			if($orderStatusRow === FALSE) {
				exit();
			}
			
			$customerID = $orderRow['OrderCustomerID'];
			$customerRecord = cartClass::getCustomerRow($customerID);
			if($customerRecord === FALSE) {
				exit();
			}

			// setup order status variables
			$subject = $orderStatusRow['OrderStatusEmailSubject'];
			$fromEmail = $orderStatusRow['OrderStatusFromEmail'];
			$textTop = $orderStatusRow['OrderStatusEmailBodyText'];
			$textTitle = 'Thank you for your order!';
			
			// replace tag with extra content;
			if($extraMessage == '') {
				$textTop = str_replace('[xxx]', '', $textTop);
			} else {
				$textTop = str_replace('[xxx]', '<br /><br />'.$extraMessage."&nbsp;&nbsp;", $textTop);		
			}			
			// get order contents
			$itemRows = cartClass::getItemRowsForOrder($orderID);
			foreach($itemRows as $itemRow) {
				$itemRecord['Product'] = cartClass::convertJSONtoArray($itemRow, 'Product');
				$itemRecord['Design'] = cartClass::convertJSONtoArray($itemRow, 'Design');
				$itemRecord['Envelope'] = cartClass::convertJSONtoArray($itemRow, 'Envelope');
				
				$productRow = productClass::getProductRow($itemRecord['Product']['ProductID']);
				if($productRow === FALSE) {
					exit();
				}
//FB::log($productRow);
				list($productDescription, $productPrice) = cartClass::getItemInfoForAdmin($productRow, $itemRecord);
				if($orderRow['OrderIsWholesale'] == 1) {
					$productPrice *= WHOLESALE_DISCOUNT;
				}
				$productHTMLContent .= '
                   		<tr>
                
                   			<td width="555" align="left" colspan="2" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:5px">'.$productDescription.'</span></td>
                   			<td width="100" align="right" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:5px">$'.number_format($productPrice, 2).'</span></td>
                   		</tr>
                    ';

				$productTextContent .= 
							$productRow['ProductSKU']."\t\t".
							$productRow['ProductName']."\t".
							$productDescription."\t".
							'$'.number_format($productPrice, 2)."\n";
			}
			
			$messageHTML = '
			<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" 
			 "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
			<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en">
				<head>
					<link rel="shortcut icon" href="favicon.ico" />
					<meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
					<title>Hampton Paper Designs</title>
					<meta name="keywords" content=""/>
					<meta name="description" content="" />
				</head>
				<body bgcolor="#ffffff" text="#666666" link="#74c7bf" vlink="#74c7bf" alink="#74c7bf">
				
				<table align="center" width="720" bgcolor="#FFFFFF" cellpadding=0 cellspacing=0 border=0>
					<tr>
						<td width="700" align="left">
							<img src="http://'.BROWSER_HTML.'admin/images/logo.gif">
							<p>
								<span style="font-family:georgia,serif; font-size:18px; letter-spacing:1px;">'.$textTitle.'</span><br /><br />
								<span style="font-family:georgia, serif; font-size:13px;">Order #: '.$orderID.'<br>
								<span style="font-family:georgia, serif; font-size:13px;">Order Date: '.date('Y-M-d', strtotime($orderRow['OrderDate'])).'<br /><br />
								Dear '.$customerRecord['CustomerBillFirstName'].' '.$customerRecord['CustomerBillLastName'].',<br /><br />
								'.$textTop.'</span><br />
							</p>
							<table width=700 style="border: 1px solid #172154" cellpadding=8 cellspacing=2>
								<tr>
									<td bgcolor="#c9eae7" width="555" align="left" valign="middle" colspan="2"><span style="font-family:georgia,serif; font-size:12px; letter-spacing:1px;padding:5px">PRODUCT DESCRIPTION</span></td>
									<td bgcolor="#c9eae7" width="100" align="right" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:5px; letter-spacing:1px;">TOTAL</span></td>
								</tr>

								'.$productHTMLContent.'
								
								<!-- discounts -->
								<!-- <tr>
									<td style="padding: 0;" id="sub_total_title" align="right" colspan="3" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 5px 0 5px;"><b>Total Order Discounts / Promos:</b></span></td>
									<td style="padding: 0 8px 0 0;" id="sub_total" align="right" width="85" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 0px 0 0;font-weight:bold;">-$</span></td>
								</tr> -->
																
								<!-- subtotal -->
								<tr>
									<td width="125">&nbsp;</td>
									<td style="padding: 0;" id="sub_total_title" align="right" width="430" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 5px 0 0;"><b>Subtotal:</b></span></td>
									<td style="padding: 0 8px 0 0;" id="sub_total" align="right" width="100" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 0px 0 0;font-weight:bold;">$'.number_format($orderRow['OrderSubTotal'], 2).'</span></td>
								</tr>
																
								<!-- tax -->
								<tr>
									<td width="125">&nbsp;</td>
									<td style="padding: 0;" id="sub_total_title" align="right" width="430" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 5px 0 0;"><b>Tax:</b></span></td>
									<td style="padding: 0 8px 0 0;" id="sub_total" align="right" width="100" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 0px 0 0;font-weight:bold;">$'.number_format($orderRow['OrderTaxTotal'], 2).'</span></td>
								</tr>
							   
								<!-- shipping -->
								<tr>
									<td width="125">&nbsp;</td>
									<td style="padding: 0;" align="right" width="430" valign="middle"><span style="font-family:georgia,serif; font-size:12px;padding:0 5px 0 0;"><b>Shipping:</b></span></td>
									<td style="padding: 0 8px 0 0;" id="shipping" align="right" width="100" valign="middle"><span style="font-family:georgia,serif; font-size:12px;padding:0 0px 0 0;font-weight:bold;">$'.number_format($orderRow['OrderShipTotal'], 2).'</span></td>
								</tr>
																
								<tr>
									<td width="125">&nbsp;</td>
									<td style="padding: 0;" id="sub_total_title" align="right" width="430" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 5px 0px 0;"><b>ORDER TOTAL:</b></span></td>
									<td style="padding: 0 8px 0 0;" id="total" align="right" width="100" valign="middle"><span style="font-family:georgia,serif; font-size:12px; padding:0 0px 0px 0;font-weight:bold;">$'.number_format($orderRow['OrderTotal'], 2).'</span></td>
								</tr>

								
								<!-- THIS IS THE LINE THAT FOLLOWS THE SUBTOTAL -->
								<tr>
									<td align="right" height="1" colspan="3"><div id="review_div_empty" ></div></td>
							   </tr>
							</table>
							
							<!-- <a style="font-family:georgia, serif; font-size:13px;" href="mailto:"></a> -->
						</td>
					</tr>
				</table>
				
				</body>
			</html>';
			
			$messageText =
						$textTitle."\n".
						'Order #: '.$orderID."\n".
						'Dear '.$customerRecord['CustomerBillFirstName'].' '.$customerRecord['CustomerBillLastName'].",\n\n".
						'Thanks for shopping with Hampton Paper Designs. '.$textTop.' Please see details below'."\n".
						'STYLE No'."\t".
						'PRODUCT NAME'."\t".
						'QTY'."\t".
						'TOTAL'."\n".
					    $productTextContent."\n".
						'Subtotal:  $'.number_format($orderRow['OrderSubTotal'], 2)."\n".
						'Tax:  $'.number_format($orderRow['OrderSubTotal'], 2)."\n".
						'Shipping:  $'.number_format($orderRow['OrderShipTotal'], 2)."\n".
						'ORDER TOTAL:  $'.number_format($orderRow['OrderTotal'], 2)."\n\n".
						"\n\n".
						'Thank you for your order!'."\n\n".
						'Hampton Paper Designs';        
			 
			// Set the message content
			$message = new Mail_mime("\n");
			$message->setTXTBody($messageText);
			$message->setHTMLBody($messageHTML);
			 
			// Get the body and headers
			$body = $message->get();
			$headers = $message->headers(array(
				'From' => $fromEmail,
				'Subject' => $subject,
	//			'CC' => 'dweiner@twcny.rr.com'
			));
			 
			// Send the message
			$mail = @Mail::factory('mail');
			$mailStatus = $mail->send($customerRecord['CustomerBillEmail'], $headers, $body);
//$mailStatus = $mail->send('dweiner@twcny.rr.com', $headers, $body);
//$mailStatus = $mail->send('david@nextinteractives.com', $headers, $body);
//FB::log($mailStatus);
//echo $messageHTML;
		}
		
	}
?>