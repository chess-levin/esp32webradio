const char OPTION_entry[] PROGMEM = 
"<option value='%i' %s>%s %s</option>";

const char MAIN_page[] PROGMEM = R"=====(
<html>
<head>
<meta http-equiv='content-type' content='text/html; charset=UTF-8'>
<meta name='viewport' content='width=320' />
<link href = "https://code.jquery.com/ui/1.10.4/themes/ui-lightness/jquery-ui.css" rel = "stylesheet">
<script src = "https://ajax.googleapis.com/ajax/libs/jquery/3.5.1/jquery.min.js"></script>

<script>
$(function() {
  $(document).ready(getAll);
  $("#btn_save").click(saveSSID);
  $("#btn_reset").click(restartHost);
  $("#btn_test").click(testStation);
  $("#btn_updt").click(updateStation);
  $("#btn_restore").click(restoreStations);
  $("#stationlist").change(getStation);
});

function getAll() {
  getSSID();
  getStationList();
}

function getStationList() {
  $.ajax({
    type:"GET",
    url:"/cmd/stations",
    data:{},
    success: function(data){
      $("#stationlist").html(data);
      getStation();
    }
  });
}

function getStation() {
  $.ajax({
    type:"GET",url:"/cmd/getstation",
    data:{"stationid":$("#stationlist").val()},
    success: function(data){
      const parts = data.split("\n");
      $("#name_input").val(parts[0]);
      $("#url_input").val(parts[1]);
      $("#enable_input").prop("checked",(parts[2] == 1));
      $("#pos_input").val(parts[3]);
    }
  });
}

function updateStation() {
  const checked = $("#enable_input").prop("checked")?1:0;
  $.ajax({
    type:"GET",
    url:"/cmd/setstation",
    data:{"stationid":$("#stationlist").val(),
          "name":$("#name_input").val(),
          "url":$("#url_input").val(),
          "enabled":checked,
          "position":$("#pos_input").val()},
    success: function(data){
      const n = ( $("#enable_input").prop("checked"))?"&#x25cf; ":"&#x2002; "
      $( "#stationlist option:selected" ).html(n + $("#name_input").val());
      alert(data);
      getStationList();
    }
  });
}

function restoreStations() {
  $.ajax({
    type:"GET",
    url:"/cmd/restorestations",
    data:{},
    success: function(data){
      getStationList();
    }
  });
}


function testStation() {
  const checked = $("#enable_input").prop("checked")?1:0;
  $.ajax({
    type:"GET",
    url:"/cmd/teststation",
    data:{"url":$("#url_input").val()},
    success: function(data){
      alert("Mit OK beenden Sie den Test");
      endTest();
    },
    error: function() {
      alert("ERROR");
    }
  });
}

function endTest() {
  const checked = $("#enable_input").prop("checked")?1:0;
  $.ajax({
    type:"GET",
    url:"/cmd/endtest",
    data:{},
    success: function(data){
    }
  });
}

function getSSID() {
  $.ajax({
    type:"GET",
    url:"/cmd/getaccess",
    data:{},
    success: function(data){
      const parts = data.split("\n");
      $("#ssid_input").val(parts[0]);
      $("#pkey_input").val(parts[1]);
      $("#ntp_input").val(parts[2]);
    }
  });
}

function saveSSID() {
  $.ajax({
    type:"GET",
    url:"/cmd/setaccess",
    data:{"ssid":$("#ssid_input").val(),"pkey":$("#pkey_input").val(),"ntp":$("#ntp_input").val()},
    success: function(data){
      alert(data);
    }
  });
}

function restartHost() {
  $.ajax({
    type:"POST",
    url:"/cmd/restart",
    data:{},
  });
}

</script>
<style>
body {
  width:300px;
  font-family:arial;
}

button {
  width:110px;
  background-color:silver;
  border-radius:10px;
}

.theme {
  background-color:cadetblue;
  padding:10px;
  border-radius: 10px;
  border: 2px solid silver;
}

.confpane {
  width:280px;
  height:135px;
}

.selectpane {
  width:280px;
  height:20px;
  margin-top:10px;
  text-align:center;
}

.editpane {
  width:280px;
  height:164px;
  margin-top:10px;
}

.txtinput {
  position:absolute;
  left:90px;
  width:200px;
}

.numinput {
  position:absolute;
  left:160px;
  width:50px;
}

.urlinput {
  width:272px;
}

label {
  color:white;
  font-family:arial;
}

</style>
</head>
<body>
<h1 style="text-align:center;">Internet Radio</h1>
<div class="confpane theme">
<div><label for="ssid_input">SSID:</label> <input id="ssid_input" class="txtinput"/></div>
<div style="padding-top:10px;"><label for="pkey_input">PKEY:</label> 
<input id="pkey_input" class="txtinput" type="password"/></div>
<div style="padding-top:10px;"><label for="ntp_input">NTP:</label> 
<input id="ntp_input" class="txtinput"/></div>
<div style="padding-top:10px;text-align:center">
<button id="btn_save" type="button">Speichern</button>
<button id="btn_reset" type="button" style="margin-left:20px;">Neustart</button></div>
<div style="padding-top:10px;text-align:center">
<button id="btn_restore" type="button" style="width:180px">Senderliste zurücksetzen</button></div>
</div>
<div class="selectpane theme">
<select id="stationlist">
</select>
</div>
<div id="config" class="editpane theme">
<div><label for="name_input">Name:</label> <input id="name_input" class="txtinput"/></div>
<div style="padding-top:10px;"><label for="url_input">URL:</label><br> <input id="url_input" class="urlinput" /></div>
<div style="padding-top:10px;">
<label for="enable_input">Verwenden:</label><input id="enable_input" class="txtinput" type="checkbox"></div>
<div style="padding-top:10px;">
<label for="pos_input">Position:</label> <input id="pos_input" type="number" step="1" min="1" max="99" size="2" class="numinput"/></div>
<div style="padding-top:20px;text-align:center">
<button id="btn_test" type="button">Testen</button>
<button id="btn_updt" type="button" style="margin-left:20px;">Ändern</button>
</div>
</div>

</body>
</html>
)=====";

//*****************************************************************************************************************
const char CONFIG_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta http-equiv='content-type' content='text/html; charset=UTF-8'>
<meta name='viewport' content='width=320' />
<style>
body {
  width:300px;
  font-family:arial;
}

button {
  width:110px;
  background-color:silver;
  border-radius:10px;
}

.theme {
  background-color:cadetblue;
  padding:10px;
  border-radius: 10px;
  border: 2px solid silver;
}

.confpane {
  width:280px;
  height:75px;
}


.txtinput {
  position:absolute;
  left:90px;
  width:200px;
}

.label {
  color:white;
  font-family:arial;
}

</style>
</head>
<body>
<h1 style="text-align:center;">Internet Radio</h1>
<form>
<div class="confpane theme">
<div><span class="label">SSID:</span> <input name="conf_ssid" class="txtinput"/></div>
<div style="padding-top:10px;"><span class="label">PKEY:</span> 
<input name="conf_pkey" class="txtinput" type="password"/></div>
<div style="padding-top:10px;text-align:center">
<button type="submit" name="reset">Neustart</button></div>
</div>
</body>
</html>
)=====";
