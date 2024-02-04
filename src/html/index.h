#pragma once

const char INDEX_HTML[] PROGMEM = R"=====(
<html>
<head>
<meta http-equiv='content-type' content='text/html; charset=UTF-8'>
<meta name='viewport' content='width=320' />

</head>
<body>
<h1 style="text-align:center;">Internet Radio</h1>
<div>

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

<form action="/wifi" method="POST" >
<div><label for="ssid_input">SSID:</label>
  <input name="ssid" id="ssid" type="text" class="txtinput"/>
</div>
<div style="padding-top:10px;"><label for="pkey_input">PKEY:</label> 
  <input name="pkey" id="pkey" class="txtinput" type="text"/>
</div>
<div style="padding-top:10px;text-align:center">
  <button id="btn_save" type="Submit">Save</button>
</div>
</form>

<div style="padding-top:10px;text-align:center">
<button onclick="window.location.href='/restart';">Restart w/o saving</button>
</div>
</div>
</body>
</html>
)=====";
