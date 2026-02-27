/*
sim-ii: Copyright (C) 2019  VetSim, Cornell University College of Veterinary Medicine Ithaca, NY

See gpl.html
*/
	var modal = {
		imgTop: '50px',
		imgPadding: '0px',
		
		textTop: '200px',
		textPadding: '20px',
		textWidth: 300,
		
		fadeDelay: 200,
		
		defaultControlWidth: 400,
		
		showText: function(content, text_align) {
			if(typeof text_align == 'undefined') {
				text_align = 'center';
			} else {
				switch(text_align) {
					case 'left':
					case 'center':
						break;
					default:
						text_align = 'center';
						break;
				}
			}
			$('#modal #modal-content').append(content);
			$('#modal .container').width(modal.textWidth);
			$('#modal .container').css({
										'height': 'auto',
										'padding': modal.textPadding,
										'text-align': text_align,
										'top': modal.textTop
			});
			$('#modal').css('position', 'fixed').fadeIn(modal.fadeDelay);
			modal.bindCloseModal();
		},

		showLogFiles: function() {
			$.ajax({
				url: BROWSER_AJAX + 'ajaxGetLogTableContent.php',
				type: 'post',
				async: false,
				data: {fn: sessionStorage.fileName},
				dataType: 'json',
				success: function(response) {
					if(response.status == AJAX_STATUS_OK) {
						modal.showModal(response);
						$('#modal .container').css({
													width: '625px',
													height: '480px',
													top: '120px',
													margin: '0 0 0 25px'
													});
						$('#modal .container .control-modal-div').css({
							width:'900px'
						});
						$('#modal-content').css('margin-top', '0');
						$('#log-table-content').css({
							height: '400px',
							overflow: 'hidden',
							'overflow-y': 'auto'
						});
						$('.close_modal').css({
							position: 'absolute',
							top: '-2px',
							right: '-2px'
						});
						
						$('table.log-table').css('width', '600px');
						
						modal.bindCloseModal();
						
						// scroll to top row position
						$('#log-table-content').scrollTop(0);
						
						$('#log-table-content table tr').click(function(evt) {
							evt.preventDefault();
							$('#log-table-content table tr').unbind();
							eventLog.url = $(this).attr('data-filename');
							
							// is there a valid video file?
							if($(this).children('td.log-video').html().search("mp4") != -1) {
								vid.url = $(this).children('td.log-video').html();
							} else {
								vid.url = '';
							}
							
							// init video
							vid.init();
							
							// init event log
							eventLog.init();
							
							$('#log-table-content table tr').removeClass('selected');
							$(this).addClass('selected');
							
							// add color classes
							$('td.event-content:contains("Setting:")').parent().addClass('settings-class');
							$('td.event-content:contains("VS:"), td.event-content:contains("Action:")').parent().addClass('simulator-class');
							$('td.event-content:contains("Comment:")').parent().addClass('comment-class');
							$('td.event-content:contains("Probe:"), td.event-content:contains("Event:"), td.event-content:contains("Trigger:")').parent().addClass('et-class');
							$('td.event-content:contains("CPR:")').parent().addClass('cpr-class');
							
							modal.closeModal();
							
							// bind checkboxes
							$('#settings-status').click(function() {
								if( $(this).hasClass('checked') == false ) {
									$('tr.settings-class').fadeOut();
									$(this).addClass('checked').html('&#10004;');
								} else {
									$('tr.settings-class').fadeIn();
									$(this).removeClass('checked').html('');
								}
							});
							
							$('#simulator-status').click(function() {
								if( $(this).hasClass('checked') == false ) {
									$('tr.simulator-class').fadeOut();
									$(this).addClass('checked').html('&#10004;');
								} else {
									$('tr.simulator-class').fadeIn();
									$(this).removeClass('checked').html('');
								}
							});
							
							$('#et-status').click(function() {
								if( $(this).hasClass('checked') == false ) {
									$('tr.et-class').fadeOut();
									$(this).addClass('checked').html('&#10004;');
								} else {
									$('tr.et-class').fadeIn();
									$(this).removeClass('checked').html('');
								}
							});
							
						});
					}
				}
			});
		},

		/********************* Utils for modals *******************/
		bindCloseModal: function() {
			$('a.close_modal, button.cancel').click(modal.closeModal);
		},
		
		showModal: function(response) {
			$('#modal-content').html(response.html);
			$('#modal').css('position', 'fixed').fadeIn(modal.fadeDelay);
			$('#modal .container').css('height', 'auto');
		},
		
		ajaxWait: function() {
			$('#modal .close_modal').hide();
			$('#modal #modal-content').append('<img class="image_modal modal_content" src="' + BROWSER_IMAGES + 'ajax_loader.gif" alt="Product Image">');
			$('#modal .container').width(modal.textWidth);
			$('#modal .container').css({
										'height': '150px',
										'padding': modal.textPadding,
										'text-align': 'center',
										'top': modal.textTop,
										'background-color': '#FFFFFF' 
			});
			$('#modal img').css('margin', 'auto');
			$('#modal').css('position', 'fixed').show();					
		},

		closeModal: function() {
			$('#modal').fadeOut(modal.fadeDelay,
				function() {
					$('#modal-content').empty();
					$('#modal .close_modal').show();
					$('#modal .container').css('width', '400px');
					$('#modal .container .control-modal-div').css('width', '345px');					
				}
			);
		},
		closeModalFast: function() {
			$('#modal').hide();
			$('#modal-content').empty();
			$('#modal .close_modal').show();
		}
	}
