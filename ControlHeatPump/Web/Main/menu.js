﻿document.getElementById("menu").innerHTML= '<a href="index.html" class="logo"></a>\
<div class="swich"><span>HP OFF</span>\
<div class="onoffswitch">\
	<input type="checkbox" name="onoffswitch" class="onoffswitch-checkbox" id="onoffswitch" onchange="swich(\'onoffswitch\')">\
	<label class="onoffswitch-label" for="onoffswitch"><span class="onoffswitch-inner"></span><span class="onoffswitch-switch"></span></label>\
</div><span>HP ON</span>\
</div><div class="menu-profiles">\
	<span id="get_mode" style="color: red">-</span><br>\
	<span id="SCH-PR"></span><br>\
	<select id="get_listprof" onchange="setParam(\'get_listProf\',\'get_listprof\');"></select>\
</div>\
<ul class="cd-accordion-menu">\
<li class="index"><a href="index.html"><i class="menu-icon menu-icon-index"></i>Состояние</a></li>\
<li class="plan"><a href="plan.html"><i class="menu-icon menu-icon-plan"></i>Схема ТН</a></li>\
<li name="USR" class="heating gvs profiles scheduler has-children">\
	<input type="checkbox" name="group-1" id="group-1">\
	<label for="group-1"><i class="menu-icon menu-icon-heat"></i>Отопление и ГВС</label>\
	<ul>\
		<li class="heating"><a href="heating.html">Отопление</a></li>\
		<li class="gvs"><a href="gvs.html">ГВС</a></li>\
		<li class="profiles"><a href="profiles.html">Профили</a></li>\
		<li class="scheduler"><a href="scheduler.html">Расписание</a></li>\
	</ul>\
</li>\
<li class="stats history has-children">\
	<input type="checkbox" name="group-2" id="group-2">\
	<label for="group-2"><i class="menu-icon menu-icon-stat"></i>Статистика</label>\
	<ul>\
		<li class="stats"><a href="stats.html">По дням</a></li>\
		<li class="history"><a href="history.html">Детально</a></li>\
	</ul>\
</li>\
<li name="USR" class="setsensors sensorst sensorsp eev relay invertor heater has-children">\
	<input type="checkbox" name="group-3" id="group-3">\
	<label for="group-3"><i class="menu-icon menu-icon-config"></i>Конфигурация ТН</label>\
	<ul>\
		<li class="sensorsp"><a href="sensorsp.html">Датчики</a></li>\
		<li class="sensorst"><a href="sensorst.html">Датчики температуры</a></li>\
		<li class="setsensors"><a href="setsensors.html">Привязка датчиков</a></li>\
		<li class="eev"><a href="eev.html">ЭРВ</a></li>\
		<li class="relay"><a href="relay.html">Реле</a></li>\
		<li class="invertor"><a href="invertor.html"><i></i>Инвертор</a></li>\
		<li class="heater"><a href="heater.html"><i></i>Котел</a></li>\
	</ul>\
</li>\
<li class="lan config system files time notice mqtt const has-children" id="MService">\
	<input type="checkbox" name="group-4" id="group-4">\
	<label for="group-4"><i class="menu-icon menu-icon-service"></i>Сервис</label>\
	<ul>\
		<li class="system"><a href="system.html">Система</a></li>\
		<li class="config"><a href="config.html">Настройки</a></li>\
		<li class="lan"><a href="lan.html">Сеть</a></li>\
		<li class="time"><a href="time.html">Дата/Время</a></li>\
		<li class="notice"><a href="notice.html">Уведомления</a></li>\
		<li class="mqtt"><a href="mqtt.html">MQTT</a></li>\
		<li class="const"><a href="const.html"><i></i>Константы</a></li>\
		<li class="files"><a href="files.html">Файлы</a></li>\
	</ul>\
</li>\
<li name="USR" class="charts test modbus log freertos has-children">\
	<input type="checkbox" name="group-5" id="group-5">\
	<label for="group-5"><i class="menu-icon menu-icon-debug"></i>Отладка ТН</label>\
	<ul>\
		<li class="charts"><a href="charts.html">Графики</a></li>\
		<li class="test"><a href="test.html">Тестирование</a></li>\
		<li class="modbus"><a href="modbus.html">Modbus</a></li>\
		<li class="log"><a href="log.html">Журнал</a></li>\
		<li class="freertos"><a href="freertos.html">ОС RTOS</a></li>\
	</ul>\
</li>\
<li class="wattrouter"><a href="wattrouter.html"><i class="menu-icon menu-icon-wr"></i>Ваттроутер</a></li>\
<li class="about"><a href="about.html"><i class="menu-icon menu-icon-about"></i>О контроллере</a></li>\
<li name="USR" id="mlogin" hidden><a href="planf.html" onclick="NeedLogin=0"><i class="menu-icon"></i>Логин</a></li>\
</ul>\
<div class="dateinfo">\
	<div id="get_status"></div>\
</div>';
var extract = new RegExp('[a-z0-9-]+\.html'); 
var pathname = location.pathname;
pathmath = pathname.match(extract);
if(!pathmath) {var activeli = document.body.className;} else {var activeli = pathmath.toString().replace(/(-set.*)?\.html/g,"");}
var elements = document.getElementsByClassName(activeli);
var countElements = elements.length;
for(i=0;i<countElements;i++){document.getElementsByClassName(activeli)[i].classList.add("active");}
loadParam("USR,get_listProf");
updateParam("get_status,get_WORK,get_MODE,get_SCHDLR(On),get_listProf_");
window.onscroll = function() { autoheight(); }
