mergeInto(LibraryManager.library, {
	js_ascii_setConsoleSize: function (cols,lines) {
	  var console=document.getElementById('console');
	  var fontWidthTest=document.getElementById('fontWidthTest');
	  var fontWidth=fontWidthTest.clientWidth;
	  var fontHeight=fontWidthTest.clientHeight;
	  console.style.width=(cols*fontWidth).toString()+'px';
	  console.style.height=(lines*fontHeight).toString()+'px';
	  console.style.maxWidth=console.style.width;
	},
	js_ascii_getCharacterWidth: function () {
	  var fontWidthTest=document.getElementById('fontWidthTest');
	  return fontWidthTest.clientWidth;
	},
	js_ascii_getCharacterHeight: function () {
	  var fontWidthTest=document.getElementById('fontWidthTest');
	  return fontWidthTest.clientHeight;
	},
	js_ascii_changeConsoleText: function(textPtr,len) {
	  var text=intArrayToString(HEAPU8.subarray(textPtr, textPtr+len));
	  setTimeout (function () {document.getElementById('console').innerHTML=text;},20);
	},
	js_ascii_changeConsoleColors: function(backColor,foreColor) {
	  var console=document.getElementById('console');
	  console.style.backgroundColor='#'+Pointer_stringify(backColor);
	  console.style.color='#'+Pointer_stringify(foreColor);
	},
	js_ascii_setTimeout: function (ms,id) {
	  fireTimeout = Module.cwrap('_onjs_fireTimeout', 'number', ['number']);
	  setTimeout (function () {fireTimeout(id);},ms);
	},
	js_ascii_onDocumentMouseKey: function (ev,isDown) {
	  if (ev!=0)
	    isMouseButtonDown=isDown&&(ev.button==0);
	},
	js_ascii_onMouseEvent: function (functionName,el,ev,buttonOverride) {
	  if (functionName!=0&&(typeof lockMouseEvent === "undefined" || lockMouseEvent==null)) {
	    var fontWidthTest=document.getElementById('fontWidthTest');
	    var fontWidth=fontWidthTest.clientWidth; //put this in some precalculated data
	    var fontHeight=fontWidthTest.clientHeight;

	    var rect = el.getBoundingClientRect();
	    var X = ev.clientX - rect.left - el.clientLeft + el.scrollLeft;
	    var Y = ev.clientY - rect.top - el.clientTop + el.scrollTop;
	    X=Math.floor(X/fontWidth);
	    Y=Math.floor(Y/fontHeight);
	    var state = (typeof isMouseButtonDown !== "undefined" && isMouseButtonDown==true);
	    if (buttonOverride>=0)
	      state=(buttonOverride>0);
	    if (X>=0&&Y>=0&&(typeof lastMouseX === "undefined" || buttonOverride>=0 || lastMouseX!=X || lastMouseY!=Y)) {
	      Module.ccall(functionName,'number',['number','number','number'],[state,X,Y]);
	      lockMouseEvent=1;
	      setTimeout (function () {lockMouseEvent=null;},10);
	      lastMouseX=X;
	      lastMouseY=Y;
	    }
	  }
	},
	js_ascii_toggleMouseMoveEvent : function (toggle) {
	  var console=document.getElementById('console');
	  var mouseMove=function (ev) {_js_ascii_onMouseEvent('_onjs_fireMouseMove',this,ev,-1);};
	  var documentMouseDown=function (ev) {_js_ascii_onDocumentMouseKey(ev,true);return false;};
	  var documentMouseUp=function (ev) {_js_ascii_onDocumentMouseKey(ev,false);};
	  if (toggle==1) {
	    document.addEventListener('mousedown',documentMouseDown,false); //this is needed to obtain a mouse button state for a mousemove event
	    document.addEventListener('mouseup',documentMouseUp,false);
	    console.addEventListener('mousemove',mouseMove,false);
	  }
	  else {
	    document.removeEventListener('mousedown',documentMouseDown,false);
	    document.removeEventListener('mouseup',documentMouseUp,false);
	    console.removeEventListener('mousemove',mouseMove,false);
	  }
	},
	js_ascii_toggleMouseKeyEvent : function (toggle) {
	  var console=document.getElementById('console');
	  var mouseDown=function (ev) {_js_ascii_onMouseEvent('_onjs_fireMouseKey',this,ev,1);return false};
	  var mouseUp=function (ev) {_js_ascii_onMouseEvent('_onjs_fireMouseKey',this,ev,0);};
	  if (toggle==1) {
	    console.addEventListener('mousedown',mouseDown,true); //Opera needs a special reminder to not select text
	    console.addEventListener('mouseup',mouseUp,false);
	  }
	  else {
	    console.removeEventListener('mousedown',mouseDown,true);
	    console.removeEventListener('mouseup',mouseUp,false);
	  }
	}
});
