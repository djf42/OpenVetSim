<?php
/*
sim-ii: 

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

	class queryClass {
				
		// define required fields from queries
		static public $productTableDescriptor = array(
			'ProductName' => array('required', 'text'),
			'ProductIsActive' => array('optional', 'checkbox:active:0:1'),
			'ProductIsPersonalizable' => array('optional', 'checkbox:active:0:1'),
			'ProductPartSKU' => array('required', 'text'),
			'ProductPriceString' => array('required', 'text'),
			'ProductBasePrice' => array('required', 'float:2'),
			'ProductBaseQty' => array('required', 'id'),
			'ProductAdditionalPrice' => array('required', 'float:2'),
			'ProductAdditionalQty' => array('required', 'int'),
			'ProductExtraDetails' => array('optional', 'text'),
			'ProductDescription' => array('optional', 'text'),
			'ProductDimension' => array('required', 'text')
		);
		
		static public $customerTableDescriptor = array(
			'CustomerShipFirstName' => array('required', 'text'),
			'CustomerShipLastName' => array('required', 'text'),
			'CustomerShipAddr1' => array('required', 'text'),
			'CustomerShipAddr2' => array('optional', 'text'),
			'CustomerShipCity' => array('required', 'text'),
			'CustomerShipStateID' => array('required', 'int'),
			'CustomerShipZip' => array('required', 'text'),
			'CustomerBillEmail' => array('required', 'text'),
			'CustomerBillFirstName' => array('required', 'text'),
			'CustomerBillLastName' => array('required', 'text'),
			'CustomerBillAddr1' => array('required', 'text'),
			'CustomerBillAddr2' => array('optional', 'text'),
			'CustomerBillCity' => array('required', 'text'),
			'CustomerBillStateID' => array('required', 'id'),
			'CustomerBillZip' => array('required', 'text'),
			'CustomerDateCreated' => array('required', 'now')
		);
		
		static public $itemTableDescriptor = array(
			'ItemProductID' => array('required', 'id'),
			'ItemOrderID' => array('required', 'id'),
			'ItemPersonalization' => array('required', 'text'),
			'ItemQty' => array('required', 'int'),
			'ItemTotalPrice' => array('required', 'float:2'),
			'ItemItemStatusID' => array('required', 'const:2'),
			'ItemDateChanged' => array('required', 'now')
		);
		
		function __construct() {

		}
		
		// get descriptors for fields
		static public function createUpdateQuery($tableName, $tableDescriptor, $postArray, $indexColumn) {
			$queryString = "UPDATE ".dbWrapper::cleanMySQLInput($tableName)." SET ";
			$queryString .= self::getQueryString($tableDescriptor, $postArray);
			$queryString .= " WHERE " . dbWrapper::cleanMySQLInput($indexColumn) . " = " . dbWrapper::cleanMySQLInput($postArray[$indexColumn]);
//dump($queryString);
			return $queryString;
		}
		
		// get descriptors for fields
		static public function createInsertQuery($tableName, $tableDescriptor, $postArray) {
			$queryString = "INSERT INTO ".dbWrapper::cleanMySQLInput($tableName)." SET ";
			$queryString .= self::getQueryString($tableDescriptor, $postArray);
//dump($queryString);
			return $queryString;
		}
		
		
		static private function getQueryString($tableDescriptor, $postArray) {
			$queryString = '';
			foreach($tableDescriptor as $fieldName => $fieldParameters) {
				if($fieldParameters[0] == 'required' || $fieldParameters[0] == 'optional') {
					$queryString .= "`".dbWrapper::cleanMySQLInput($fieldName)."` = ";
					switch ($fieldParameters[1]) {
						case 'id':
							$queryString .= dbWrapper::cleanMySQLInput($postArray[$fieldName]).', ';
							break;
						case 'int':
							$queryString .= dbWrapper::cleanMySQLInput($postArray[$fieldName]).', ';
							break;
						case 'text':
							$queryString .= "'".dbWrapper::cleanMySQLInput($postArray[$fieldName])."', ";
							break;
						case 'sha256':
							$queryString .= "'".hash("sha256", dbWrapper::cleanMySQLInput($postArray[$fieldName]))."', ";
							break;
						case 'now':
							$queryString .= "NOW(), ";
							break;
						default:
							$fieldType = explode(':', $fieldParameters[1]);
							if($fieldType[0] == 'const') {
								$queryString .= $fieldType[1].", ";
							} else if($fieldType[0] == 'checkbox') { // format: 'checkbox:valueIfChecked:valueIfCheckedForDB:valueIfNotCheckedForDB'
								$queryString .= (isset($postArray[$fieldName]) == true 
												&& $postArray[$fieldName] == $fieldType[1]) 
													? $fieldType[3].', ' 
													: $fieldType[2].', ';				
							} else if($fieldType[0] == 'float') { // format: 'float:x' where x = number of decimal places
								$queryString .= number_format(dbWrapper::cleanMySQLInput($postArray[$fieldName]), $fieldType[1]).', ';
							}
							break;
					}
				}
			}
			// last comma
			return substr($queryString, 0, -2);
		}
		
		static public function updateProductSubCategories($postArray) {
			// validate that prod ID is valid
			if(adminClass::getProductRow($postArray['ProductID']) == FALSE) {
				return FALSE;
			}
			// delete subcategories associated with product
			$result = dbWrapper::dbDeleteQueryResult("
				DELETE FROM ProductSubCategory WHERE ProductSubCategoryProductID = ".$postArray['ProductID']
			);
			if($result == FALSE) {
				return FALSE;
			}
			
			// get all subcategory listings
			$subCatArray = array();
			foreach($postArray as $key => $value) {
				if(strpos($key, 'sc_') !== FALSE) {
					$tmp = explode('sc_', $key);
					$subCatArray[] = $tmp[1];
				}
			}
			
			// rewrite subcategories
			if(count($subCatArray) > 0) {
				foreach($subCatArray as $subCatIndex) {
					// get count of how many entries there are for product sub category, then set
					// index to count + 1
					
					$subCatResult = dbWrapper::dbSelectQueryResult("
										SELECT * FROM ProductSubCategory
										WHERE `ProductSubCategorySubCategoryID` = $subCatIndex
										ORDER BY ProductSubCategoryIndex DESC
										LIMIT 1
					");
					
					if(count($subCatResult) == 0) {
						$index = 1;
					} else {
						$index = $subCatResult[0]['ProductSubCategoryIndex'] + 1;
					}
					$result = dbWrapper::dbInsertQueryResult("
										INSERT INTO ProductSubCategory
										SET `ProductSubCategoryProductID` = ".dbWrapper::cleanMySQLInput($postArray['ProductID']).", 
										`ProductSubCategorySubCategoryID` = $subCatIndex,
										`ProductSubCategoryIndex` = $index
										");
				}
			}
			return TRUE;
		}
		
		static public function updateRelatedProducts($postArray) {
			// validate that prod ID is valid
			if(adminClass::getProductRow($postArray['ProductID']) == FALSE) {
				return FALSE;
			}
			$relatedProductsArray = json_decode($_POST['relatedProductsOrder']);
			
			// delete entries
			$result = dbWrapper::dbDeleteQueryResult("
										DELETE FROM ProductAlsoLike 
										WHERE ProductID = ".$postArray['ProductID']
			);
			if($result == FALSE) {
				return FALSE;
			}
			
			// cycle through any new entries
			if(count($relatedProductsArray) > 0) {
				$index = 1;
				foreach($relatedProductsArray as $relatedProductString) {
					$relatedProductID = explode('ppsku_', $relatedProductString);
					$result = dbWrapper::dbInsertQueryResult("
										INSERT INTO ProductAlsoLike
										SET `ProductID` = ".dbWrapper::cleanMySQLInput($postArray['ProductID']).", 
										`ProductAlsoLikeProductID` = ".$relatedProductID[1].",
										`ProductAlsoLikeIndex` = $index
										");
					$index++;
				}
			}
			return TRUE;
		}
		
		static public function updateProductImageOrder($postArray) {
			// validate that prod ID is valid
			if(adminClass::getProductRow($postArray['ProductID']) == FALSE) {
				return FALSE;
			}
			
			$imageOrderArray = json_decode($_POST['productImagesOrder']);
			// cycle through any new entries
			if(count($imageOrderArray) > 0) {
				$index = 1;
				foreach($imageOrderArray as $imageOrderString) {
					$productImageID = explode('pimg_', $imageOrderString);
					$result = dbWrapper::dbUpdateQueryResult("
										UPDATE `ProductImage`
										SET `ProductImageIndex` = ".$index." 
										WHERE `ProductImageID` = ".dbWrapper::cleanMySQLInput($productImageID[1])
										);
					if($result == FALSE) {
						return FALSE;
					}
					$index++;
				}
			}
			return TRUE;
		}
		
		static public function deleteProductImages($postArray) {
			// validate that prod ID is valid
			if(adminClass::getProductRow($postArray['ProductID']) == FALSE) {
				return FALSE;
			}
			
			// get all subcategory listings
			$delImageArray = array();
			foreach($postArray as $key => $value) {
				if(strpos($key, 'di_') !== FALSE) {
					$tmp = explode('di_', $key);
					$delImageArray[] = $tmp[1];
				}
			}
			if($delImageArray > 0) {
				foreach($delImageArray as $delImageID) {
					$result = dbWrapper::dbSelectQueryResult("
											SELECT * FROM ProductImage
											WHERE ProductImageID = ".dbWrapper::cleanMySQLInput($delImageID)
											);
					
					// verify that image belongs to product before we delete it.
					if(count($result) == 0) {
						return FALSE;
					}
					if($result[0]['ProductImageProductID'] != $postArray['ProductID']) {
						return FALSE;
					}
					
					// delete file from server
					@unlink(SERVER_PRODUCTS.$result[0]['ProductImageURL']);

					// delete from database
					$result = dbWrapper::dbDeleteQueryResult("
											DELETE FROM ProductImage
											WHERE ProductImageID = ".dbWrapper::cleanMySQLInput($delImageID)
											);	
				}
			}
			return TRUE;
		}
		
		static public function addProductImages($postArray, $fileArray) {
			// validate that prod ID is valid
			if(adminClass::getProductRow($postArray['ProductID']) == FALSE) {
				return FALSE;
			}
			
			if(count($fileArray) == 0) {
				return TRUE;
			}
			
			foreach($fileArray as $fHandle) {
				switch ($fHandle["type"])
				{
					case "image/jpeg":
					case "image/pjpeg":
					case "image/png":
					case "image/gif":
						break;
					default:
						continue;
				} // end switch
				
				// upload file and rename...
				if($fHandle['error'] === UPLOAD_ERR_OK && $fHandle['size'] > 0) {
					// get last image file index
					$result = dbWrapper::dbSelectQueryResult("
											SELECT * FROM `ProductImage`
											WHERE `ProductImageProductID` = ".dbWrapper::cleanMySQLInput($postArray['ProductID'])."
											ORDER BY `ProductImageIndex` DESC
											LIMIT 1
											");
					if(count($result) == 0) {
						$index = 1;
					} else {
						$index = $result[0]['ProductImageIndex'] + 1;
					}

					$imgFileNameInfo = pathinfo($fHandle['name']);
					$imgFileName = SERVER_PRODUCTS.$imgFileNameInfo['filename']."~".$postArray['ProductID']."_".time().".".$imgFileNameInfo['extension'];
					$imgFileNameURL = $imgFileNameInfo['filename']."~".$postArray['ProductID']."_".time().".".$imgFileNameInfo['extension'];
					if(move_uploaded_file($fHandle["tmp_name"], $imgFileName) == FALSE) {
						continue;
					}
					chmod($imgFileName, 0644);
					
					// now add to database
					$result = dbWrapper::dbInsertQueryResult("
											INSERT INTO `ProductImage` 
											SET `ProductImageProductID` = ".dbWrapper::cleanMySQLInput($postArray['ProductID']).",
											ProductImageIndex = $index,
											ProductImageURL = '$imgFileNameURL'
					");
					continue;
				} else if($fHandle['error'] === UPLOAD_ERR_NO_FILE) {
					continue;		
				} else {
					continue;
				}
			}
			return TRUE;
		}
		
		static public function importProductRecord($prodData) {
			$result = dbWrapper::dbSelectQueryResult("
														SELECT * FROM Product
														WHERE ProductPartSKU = '".dbWrapper::cleanMySQLInput($prodData['ProductPartSKU'])."'
													");
			if(count($result) != 0) {
				return FALSE;
			}
			$insertResult = dbWrapper::dbInsertQueryResult(queryClass::createInsertQuery('Product', queryClass::$productTableDescriptor, $prodData));
			if($insertResult == FALSE) {
				return FALSE;
			}
			$subCatResult = dbWrapper::dbInsertQueryResult("
														INSERT INTO ProductSubCategory
														SET ProductSubCategoryProductID = $insertResult,
														ProductSubCategorySubCategoryID = ".DEFAULT_SUB_CAT_ID.",
														ProductSubCategoryIndex = 1
			");
			return TRUE;
		}
	}
?>