function loadBanner()
{
    $.ajax({ type: "GET", url: "/api$banner", dataType: "html" }).done( function( response ) {
	document.getElementById( "banner" ).innerHTML = response;
    });
}
