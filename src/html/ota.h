#pragma once

const char OTA_HTML[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta http-equiv='content-type' content='text/html; charset=UTF-8'>
<meta name='viewport' content='width=320' />

</head>
<body>
<h1 style="text-align:center;">Internet Radio</h1>
<div style="text-align:center;">

<div id="err"></div>

<script>
  const queryString = window.location.search;
  const urlParams = new URLSearchParams(queryString);
  if (urlParams.has('error')) {
    const error = urlParams.get('error');
    const div = document.getElementById('err');
    var p = document.createElement('p');
    p.style.color = 'black';
    p.textContent = "Input data error:" + error;
    div.appendChild(p);
  }
</script>

<form action="/ota" enctype='multipart/form-data' method="POST" >
<div><label for="bin">Firmware:</label>
  <input name="bin" id="bin" type="file" class="txtinput"/>
</div>

<div style="padding-top:10px;text-align:center">
  <button id="btn_save" type="Submit">Update</button>
</div>
</form>

<div style="padding-top:10px;text-align:center">
<button onclick="window.location.href='/restart';">Restart w/o Updating</button>
</div>
</div>
</body>
</html>
)=====";
