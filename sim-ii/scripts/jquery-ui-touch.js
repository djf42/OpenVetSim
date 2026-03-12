/**
 * jquery-ui-touch.js
 * Touch event shim for jQuery UI sliders.
 *
 * jQuery UI slider widgets are built entirely on mouse events (mousedown /
 * mousemove / mouseup).  Touch screens fire touch events instead, so the
 * sliders don't respond to finger drags without this shim.
 *
 * Strategy:
 *   1. Intercept touchstart / touchmove / touchend on the document.
 *   2. When the touch originates inside a .ui-slider, synthesise the
 *      equivalent MouseEvent and dispatch it so jQuery UI's handlers fire.
 *   3. Use { passive: false } on all three listeners so we can call
 *      preventDefault() and prevent the page from scrolling while the user
 *      drags a slider.
 */
(function ($) {
    'use strict';

    var sliderDragging = false;

    /**
     * Build and dispatch a MouseEvent.
     *
     * @param {string}       type    - e.g. 'mousedown', 'mousemove', 'mouseup'
     * @param {Touch}        touch   - the Touch object with coordinates
     * @param {EventTarget}  target  - element to dispatch the event on
     */
    function simulateMouse(type, touch, target) {
        var evt = new MouseEvent(type, {
            bubbles:    true,
            cancelable: true,
            view:       window,
            button:     0,
            buttons:    (type === 'mouseup') ? 0 : 1,
            clientX:    touch.clientX,
            clientY:    touch.clientY,
            screenX:    touch.screenX,
            screenY:    touch.screenY,
        });
        target.dispatchEvent(evt);
    }

    // touchstart — only activate when touch begins inside a slider
    document.addEventListener('touchstart', function (e) {
        if (!$(e.target).closest('.ui-slider').length) return;
        sliderDragging = true;
        simulateMouse('mousedown', e.touches[0], e.target);
        e.preventDefault();   // prevent scroll / tap-highlight while on slider
    }, { passive: false });

    // touchmove — route to document so jQuery UI's document-level mousemove
    // handler (installed when mousedown fires) picks it up correctly
    document.addEventListener('touchmove', function (e) {
        if (!sliderDragging) return;
        simulateMouse('mousemove', e.touches[0], document);
        e.preventDefault();   // prevent scroll while dragging
    }, { passive: false });

    // touchend — release the drag
    document.addEventListener('touchend', function (e) {
        if (!sliderDragging) return;
        sliderDragging = false;
        simulateMouse('mouseup', e.changedTouches[0], document);
    }, { passive: false });

}(jQuery));
