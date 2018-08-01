
var source = new EventSource( '/api$events' );

source.addEventListener( 'tick', function( e ) {

    var obj = JSON.parse( e.data );

    var minutes = ~~(obj.tick.time / 60);
    var seconds = obj.tick.time % 60;
    $('#elapsed-time').html( minutes + "'" + seconds + "\"" + " " + obj.tick.nsec + "ns" );
    $('#temp').html( obj.tick.temp );
    var time_since_start = Math.round( obj.tick.time_since_start * 100 ) / 100;
    $('#time-since_start').html( time_since_start );

});

source.addEventListener( 'wave', function( e ) {

    var obj = JSON.parse( e.data );

    d3.select( '#plot' ).selectAll( 'svg' ).remove();
    var svg = d3.select('#plot').append("svg").attr( "width", 800 ).attr("height", 400);
    
    var xIncrement = obj.waveform.meta.xIncrement * 1e6;
    var xMin = obj.waveform.meta.xMin * 1e6;
    var xMax = obj.waveform.meta.xMax * 1e6;
    
    //console.log( obj.waveform.meta );
    var x_scale = d3.scaleLinear().domain([xMin, xMax]).range( [10, 780] );
    var y_scale = d3.scaleLinear().domain([-2000,2000]).range([380, 10]);

    var line = d3.line()
        .x(function(d,i) { return x_scale( i * xIncrement + xMin ); })
        .y(function(d) { return y_scale( d ); });

    //console.log( xy );
    //console.log( obj.waveform.data );
    svg.append( 'rect' )
        .attr( 'wdith', "100%" )
        .attr( 'height', "100%" )
        .attr( "fill", "navy" );
    
    var path = svg.append("path").attr("class", "line")
        .attr("d", line( obj.waveform.data ))
        .style("fill", "none")
        .style("stroke", "lime")
        .style("stroke-width", 1 );
    
    svg.append("text")
        .attr("x", 0).attr("y", 10)
        .attr("font-size", "12px")
        .attr("class", "title")
        .text( JSON.stringify( obj.waveform.meta ) )
        .style( 'fill', 'pink' );
    

});
