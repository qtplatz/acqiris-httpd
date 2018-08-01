
function downloadOnload() {
    var element = document.createElement("script");
    //element.src = "/defer.js";
    //document.body.appendChild(element);
    loadBanner();
}

if ( window.addEventListener )
    window.addEventListener( "load", downloadOnload );
else if ( window.attachEvent)
    window.attachEvent( "load", downloadOnload);
