function testClick(nodeId)
{
	var mytext = document.createTextNode("I was appended by the recently added javascript file - js1.js.");
	document.getElementById(nodeId).appendChild(mytext);
}