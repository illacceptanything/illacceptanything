function showText(nodeId)
{
	var mytext = document.createTextNode("I was appended by the recently added javascript file - js2.js.");
	document.getElementById(nodeId).appendChild(mytext);
}