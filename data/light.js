/* Filename: custom.js */

$(document).ready(function()
{

  var val_lux, val_GODZINA, val_MINUTA, val_SEKUNDA, val_STREFA;

  function pad (str, max) {
    str = str.toString();
    return str.length < max ? pad("0" + str, max) : str;
  }

  $.getJSON('/esp_data', function(md)
    {
      $( "#red" ).slider( "value", md.fdbk_tgtRED );
      handle_red.text( $( "#red" ).slider( "value" ) );
      $( "#green" ).slider( "value", md.fdbk_tgtGREEN );
      handle_green.text( $( "#green" ).slider( "value" ) );
      $( "#blue" ).slider( "value", md.fdbk_tgtBLUE );
      handle_blue.text( $( "#blue" ).slider( "value" ) );
      $( "#slider" ).slider( "value", md.fdbk_tgtLUX );
      handle_lux.text( $( "#slider" ).slider("value") );
      $( "#chid" ).html( '<a><div id="left">Chip ID:</div><div id="right">' + md.chid + '</div></a>');
      $( "#freeheap" ).html( '<a><div id="left">Free Heap:</div><div id="right">' + md.freeheap + '</div></a>');
      $( "#flashchid" ).html( '<a><div id="left">Flash Chip ID:</div><div id="right">' + md.flashchid + '</div></a>');
      $( "#flashchrealsize" ).html( '<a><div id="left">Flash Real Size:</div><div id="right">' + md.flashchrealsize + ' bytes</div></a>');
      $( "#flashchsize" ).html( '<a><div id="left">Flash Size:</div><div id="right">' + md.flashchsize + ' bytes</div></a>');
      $( "#flashchspeed" ).html( '<a><div id="left">Flash Speed:</div><div id="right">' + md.flashchspeed + ' MHz</div></a>');
      $( "#flashchmode" ).html( '<a><div id="left">Flash Mode:</div><div id="right">' + md.flashchmode + '</div></a>');
      $( "#flashchsizebyid" ).html( '<a><div id="left">Flash Size by ID:</div><div id="right">' + md.flashchsizebyid + ' bytes</div></a>');
      $( "#bootmode" ).html( '<a><div id="left">Boot mode:</div><div id="right">' + md.bootmode + '</div></a>');
      $( "#sketchsize" ).html( '<a><div id="left">Sketch Size:</div><div id="right">' + md.sketchsize + ' bytes</div></a>');
      $( "#freesketchspace" ).html( '<a><div id="left">Free Sketch Space:</div><div id="right">' + md.freesketchspace + ' bytes</div></a>');
      $( "#IP" ).html( '<a><div id="left">IP:</div><div id="right">' + md.IP + '</div></a>');
      $( "#cpufreq" ).html( '<a><div id="left">CPU Frequency:</div><div id="right">' + md.cpufreq + ' MHz</div></a>');
      $( "#W_START_HOUR_VAL" ).val( md.s_W_START_HOUR_VAL );
      $( "#W_START_MIN_VAL" ).val( md.s_W_START_MIN_VAL );
      $( "#W_STOP_HOUR_VAL" ).val( md.s_W_STOP_HOUR_VAL );
      $( "#W_STOP_MIN_VAL" ).val( md.s_W_STOP_MIN_VAL );
      $( "#RGB_START_HOUR_VAL" ).val( md.s_RGB_START_HOUR_VAL );
      $( "#RGB_START_MIN_VAL" ).val( md.s_RGB_START_MIN_VAL );
      $( "#RGB_STOP_HOUR_VAL" ).val( md.s_RGB_STOP_HOUR_VAL );
      $( "#RGB_STOP_MIN_VAL" ).val( md.s_RGB_STOP_MIN_VAL );
      $( "#TDEL_AUTOOFF" ).val( md.fdbk_TIMER_LIGHT_OFF_SETPOINT );
      refreshSwatch();
    });

  /***********************TABS SECTION***********************/
  $(function() {
    $( "#tabs" ).tabs();
  });

  $("#tabs").height(screen.height*0.7);
  $("#tabs").width(screen.width*0.6);
  /**********************************************************/

  /*******************AUTO REFRESH SECTION*******************/
	function doRefresh(){
  	$.getJSON('/esp_data', function(jd)
  	{
  	  val_lux=Number(jd.lux);
  	  val_GODZINA=Number(jd.godzina);
  	  val_MINUTA=Number(jd.minuta);
  	  val_SEKUNDA=Number(jd.sekunda);
  	  val_STREFA=Number(jd.strefa);
  	  fdbk_txtLUX = Number(jd.fdbk_tgtLUX);
      $("#progressbar-value").text(val_lux + 'lx');
  	  $("#progressbar").progressbar( "value", Number(jd.lux));
  	  $( "#cycletime" ).html( '<a><div id="left">Cycle Time:</div><div id="right">' + jd.cycletime + ' us</div></a>');
  	  $( "#pir" ).html( '<a><div id="left">Ruch:</div><div id="right">' + jd.pir + '</div></a>');
  	  $( "#lux" ).html( '<a><div id="left">Jasnosc:</div><div id="right">' + jd.lux + ' lx</div></a>');
  	  $( "#cztem" ).html( '<a><div id="left">Temperatura:</div><div id="right">' + jd.cztem + ' °C</div></a>');
  	  $( "#czhum" ).html( '<a><div id="left">Wilgotnosc:</div><div id="right">' + jd.czhum + ' %</div></a>');

      $( "#clock" ).html('<a>' + pad(jd.godzina,2) + ':' + pad(jd.minuta,2) + ':' + pad(jd.sekunda,2) + '</a>');

      //$( "#valueslide_red" ).text( $( "#red" ).slider( "value" ) );
      //$( "#valueslide_green" ).text( $( "#green" ).slider( "value" ) );
      //$( "#valueslide_blue" ).text( $( "#blue" ).slider( "value" ) );
    });
  }

  $(function() {
      setInterval(doRefresh, 500);
    });
  /**********************************************************/

  /******************RANDOM FUNCTION SECTION*****************/

  function przeslij() {
    $.getJSON('/setESPval?tgtLUX=' + $( "input:first" ).val(), function() {});
    $( "#slider" ).slider( "value", $( "input:first" ).val() );
    $( "#custom-handle" ).text($( "input:first" ).val());
    $( "#dialog" ).dialog( "close" );
  }
  function przeslij_czas() {
    $.getJSON('/setESPval?SAVE_TIME_DATA=1&s_GODZINA=' + $( "#T_HOUR_VAL" ).val() + '&s_MINUTA=' + $( "#T_MIN_VAL" ).val() + '&s_SEKUNDA=' + $( "#T_SEC_VAL" ).val() + '&s_STREFA=' + $( "#T_TIMEZONE_VAL" ).val(), function() {});
    $( "#dialog_czas" ).dialog( "close" );
  }
  function zapisz_UTC_EEPROM() {
    $.getJSON('/setESPval?TIME_EEPROM_WRITE_DATA=1', function() {});
  }

  $( "#dialog" ).dialog({
    autoOpen: false,
    show: {
      effect: "blind",
      duration: 200,
      function(){
      $( "input:first" ).attr('value',val_lux);
      }
    },
    buttons: {
        "Przeslij": przeslij
    },
    hide: {
      effect: "explode",
      duration: 200
    }
  });

  $( "#dialog_czas" ).dialog({
    autoOpen: false,
    show: {
      effect: "blind",
      duration: 200,
      function(){
      $( "input:first" ).attr('value',val_lux);
      }
    },
    buttons: {
        "Przeslij": przeslij_czas,
        "UTC do EEPROM": zapisz_UTC_EEPROM
    },
    hide: {
      effect: "explode",
      duration: 200
    }
  });

  $(".moc-container").click(function() {
    $( "#dialog" ).dialog( "open" );
  });

  $("#T_SET").click(function() {
    $( "#dialog_czas" ).dialog( "open" );
  });

  /**********************************************************/


  /********************RGB SLIDER SECTION********************/
  function hexFromRGB(r, g, b) {
    var hex = [
      r.toString( 16 ),
      g.toString( 16 ),
      b.toString( 16 )
    ];
    $.each( hex, function( nr, val ) {
      if ( val.length === 1 ) {
        hex[ nr ] = "0" + val;
      }
    });
    return hex.join( "" ).toUpperCase();
  }

  function refreshSwatch() {
    var red = $( "#red" ).slider( "value" ),
      green = $( "#green" ).slider( "value" ),
      blue = $( "#blue" ).slider( "value" ),
      hex = hexFromRGB( red, green, blue );
    $( "#swatch" ).css( "background-color", "#" + hex );
  }


    $( "#red, #green, #blue" ).slider({
      orientation: "horizontal",
      range: "min",
      max: 255
    });

    var handle_red = $( "#valueslide_red" );
    $( "#red" ).slider({
      create: function( event, ui ) {
        refreshSwatch();
      },
      stop: function( event, ui ) {
        $.getJSON('/setESPval?RED=' + $( this ).slider( "value" ), function() {});
        handle_red.text( $( this ).slider( "value" ) );
        refreshSwatch();
      },
      slide: function( event, ui ) {
        handle_red.text( ui.value );
        refreshSwatch();
      },
      change: refreshSwatch()
    });

    var handle_green = $( "#valueslide_green" );
    $( "#green" ).slider({
      create: function() {
        refreshSwatch();
      },
      stop: function( event, ui ) {
        $.getJSON('/setESPval?GREEN=' + $( this ).slider( "value" ), function() {});
        handle_green.text( $( this ).slider( "value" ) );
        refreshSwatch();
      },
      slide: function( event, ui ) {
        handle_green.text( ui.value );
        refreshSwatch();
      },
      change: refreshSwatch()
    });

    var handle_blue = $( "#valueslide_blue" );
    $( "#blue" ).slider({
      create: function() {
        refreshSwatch();
      },
      stop: function( event, ui ) {
        $.getJSON('/setESPval?BLUE=' + $( this ).slider( "value" ), function() {});
        handle_blue.text( $( this ).slider( "value" ) );
        refreshSwatch();
      },
      slide: function( event, ui ) {
        handle_blue.text( ui.value );
        refreshSwatch();
      },
      change: refreshSwatch()
    });

  /**********************************************************/

    var handle_lux = $( "#custom-handle" );
    $( "#slider" ).slider({
      create: function() {

      },
      slide: function( event, ui ) {
        handle_lux.text( ui.value );
      },
      stop: function( event, ui ) {
        $.getJSON('/setESPval?tgtLUX=' + $( this ).slider( "value" ), function() {});
        handle.text( $( this ).slider( "value" ) );
      },
      max: 65535
    });

  /**********************************************************/

    $( "#progressbar" ).progressbar({
      max: 65535,
    });

  $( "#LEDON" ).click(function() {
    $.getJSON('/setESPval?LEDON=1', function() {});
  });
  $( "#LEDOFF" ).click(function() {
    $.getJSON('/setESPval?LEDON=0', function() {});
  });
  $( "#RAINBOW" ).click(function() {
    $.getJSON('/setESPval?RAINBOW=1', function() {});
  });

  $( "#SAVE_SETTINGS" ).click(function() {
    $.getJSON('/setESPval?SAVE_LIGHT_DATA=1', function() {});
  });

  $('#LUX_VAL').bind('keypress keydown keyup', function(e){
    if(e.keyCode == 13) { e.preventDefault(); }
  });

  $( "#SYNC" ).click(function() {
    $.getJSON('/setESPval?SYNC=1', function() {});
  });

  $( "#DOWNLOAD_TIMES" ).click(function() {
    $.getJSON('/esp_data', function(sd)
      {
        $( "#W_START_HOUR_VAL" ).val( sd.s_W_START_HOUR_VAL );
        $( "#W_START_MIN_VAL" ).val( sd.s_W_START_MIN_VAL );
        $( "#W_STOP_HOUR_VAL" ).val( sd.s_W_STOP_HOUR_VAL );
        $( "#W_STOP_MIN_VAL" ).val( sd.s_W_STOP_MIN_VAL );
        $( "#RGB_START_HOUR_VAL" ).val( sd.s_RGB_START_HOUR_VAL );
        $( "#RGB_START_MIN_VAL" ).val( sd.s_RGB_START_MIN_VAL );
        $( "#RGB_STOP_HOUR_VAL" ).val( sd.s_RGB_STOP_HOUR_VAL );
        $( "#RGB_STOP_MIN_VAL" ).val( sd.s_RGB_STOP_MIN_VAL );
      });
  });
  $( "#SAVE_TIMES" ).click(function() {
    $.getJSON('/setESPval?RGBW_EEPROM_WRITE_DATA=1&s_W_START_HOUR_VAL=' +
                                                    $( "#W_START_HOUR_VAL" ).val() +
                                                    '&s_W_START_MIN_VAL=' +
                                                    $( "#W_START_MIN_VAL" ).val() +
                                                    '&s_W_STOP_HOUR_VAL=' +
                                                    $( "#W_STOP_HOUR_VAL" ).val() +
                                                    '&s_W_STOP_MIN_VAL=' +
                                                    $( "#W_STOP_MIN_VAL" ).val() +
                                                    '&s_RGB_START_HOUR_VAL=' +
                                                    $( "#RGB_START_HOUR_VAL" ).val() +
                                                    '&s_RGB_START_MIN_VAL=' +
                                                    $( "#RGB_START_MIN_VAL" ).val() +
                                                    '&s_RGB_STOP_HOUR_VAL=' +
                                                    $( "#RGB_STOP_HOUR_VAL" ).val() +
                                                    '&s_RGB_STOP_MIN_VAL=' +
                                                    $( "#RGB_STOP_MIN_VAL" ).val()
                                                    , function() {});
  });

  $( "#SAVE_TDEL_AUTOOFF" ).click(function() {
    $.getJSON('/setESPval?TDEL_AUTOOFF=' + $( "#TDEL_AUTOOFF" ).val(), function() {});
  });

  $( "#DOWNLOAD_TDEL_AUTOOFF" ).click(function() {
    $.getJSON('/esp_data', function(xd)
      {
        $( "#TDEL_AUTOOFF" ).val( xd.fdbk_TIMER_LIGHT_OFF_SETPOINT );
      });
  });

  $( "#dialog" ).on( "dialogopen", function( event, ui ) {
    $( "#LUX_VAL" ).val( val_lux );
  });

  $( "#dialog_czas" ).on( "dialogopen", function( event, ui ) {
    $( "#T_HOUR_VAL" ).val( val_GODZINA );
    $( "#T_MIN_VAL" ).val( val_MINUTA );
    $( "#T_SEC_VAL" ).val( val_SEKUNDA );
    $( "#T_TIMEZONE_VAL" ).val( val_STREFA );
  });

});
