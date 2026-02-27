/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	
	// user functions
	var user = {
		// update user profile
		updateUser: function(userID) {
		
			// quck check to see if form is valid
			if(user.validateUserRecord(userID) == false) {
				alert('Please correct highlighted fields!!');
			} else {
				$.ajax({
					url: BROWSER_AJAX + 'ajaxUpdateUser.php',
					type: 'post',
					async: false,
					dataType: 'json',
					data: {uID: userID, 
							fn: $('input[name=UserFirstName-' + userID + ']').val(),
							ln: $('input[name=UserLastName-' + userID + ']').val(),
							em: $('input[name=UserEmail-' + userID + ']').val(),
							pw: $('input[name=UserPassWord-' + userID + ']').val()
						},
					success: function(response) {
						if(response.status == AJAX_STATUS_OK) {
							alert('Your changes have been saved');
							modal.closeModal();
						}
					}
				});
			}
		},
		
		setError: function(fieldName) {
			$('input[name=' + fieldName + ']').addClass('error');
		},
		
		clearError: function(fieldName) {
			$('input[name=' + fieldName + ']').removeClass('error');
		},
		
		validateUserRecord: function(userID) {
			var formValid = true;
			if($('UserFirstName-' + userID).val() == '') {
				user.setError('UserFirstName-' + userID);
				formValid = false;
			} else {
				user.clearError('UserFirstName-' + userID);			
			}
			
			if($('UserLastName-' + userID).val() == '') {
				user.setError('UserLastName-' + userID);
				formValid = false;
			} else {
				user.clearError('UserLastName-' + userID);			
			}
						
			var emailPattern = /^[a-zA-Z0-9._-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,4}$/;  
			if(!emailPattern.test($('input[name=UserEmail-' + userID + ']').val())) {
				user.setError('UserEmail-' + userID);
				formValid = false;
			} else {
				user.clearError('UserEmail-' + userID);
			} 
			return formValid;
		}
	}