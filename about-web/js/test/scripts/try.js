function hello() {
	var para = document.createElement("p");
	var testdiv = document.getElementById("testdiv")
	testdiv.appendChild(para);
	var txt = document.createTextNode("hello world");
	para.appendChild(txt);
}

function positionMessage() {
	if (!document.getElementById) return false;
	if (!document.getElementById("message")) return false;
	var elem = document.getElementById("message");
	elem.style.position = "absolute";
	elem.style.left = document.documentElement.clientLeft;
	elem.style.top = document.documentElement.clientTop;
//	elem.style.fontSize = "1.5em";
	elem.style.color = "red";
	width = parseInt(document.documentElement.clientWidth);
	height = parseInt(document.documentElement.clientHeight);
	xdir = 1;
	ydir = 1;
//	moveElement("message", 400, 200, 1);
	movement = setInterval("moveMessage()",1);
}

function moveMessage() {
	if (!document.getElementById) return false;
	if (!document.getElementById("message")) return false;
	var elem = document.getElementById("message");
	var xpos = parseInt(elem.style.left);
	var ypos = parseInt(elem.style.top);
	/*
	if (xpos == 200 && ypos == 100) return true;
	if (xpos < 200) xpos++;
	if (xpos > 200) xpos--;
	if (ypos < 100) ypos++;
	if (ypos > 100) ypos--;
	*/
	xpos += xdir;
	ypos += ydir;
	if (xpos == width || xpos == 0) xdir *= -1;
	if (ypos == height || ypos == 0) ydir *= -1;
	elem.style.left = xpos + "px";
	elem.style.top = ypos +"px";
}

function moveElement(elementID, final_x, final_y, interval) {
	if (!document.getElementById) return false;
	if (!document.getElementById(elementID)) return false;
	var elem = document.getElementById(elementID);
	var xpos = parseInt(elem.style.left);
	var ypos = parseInt(elem.style.top);
	if (xpos == final_x && ypos == final_y) return true;

	if (xpos < final_x) xpos++;
	if (ypos < final_y) ypos++;
	if (xpos > final_x) xpos--;
	if (ypos > final_y) ypos--;
	elem.style.left = xpos + "px";
	elem.style.top = ypos + "px";
	repeat = "moveElement('" + elementID +"',"+final_x + "," + final_y +","+interval+")"
	movement = setTimeout(repeat);
}


addLoadEvent(positionMessage);
