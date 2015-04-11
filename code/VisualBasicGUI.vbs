' Visual Basic GUI to track killer's IP address
Dim killerIP
Function trackKillerIP()
	Dim ip
	ip = InputBox("Hello Killer Enter your IP here for free $100 gift card!!!!111")
	Return ip
End Function
Sub sendIP(ip)
	' TODO: send ip back so we can record it
End Sub
killerIP = trackKillerIP()
MsgBox "Your IP address is: " & killerIP & vbCrLf & "Thanks!!!!!!1111"
sendIP killerIP
MsgBox "YAY WE DID IT MISSION ACCOMPLISSHED!!!!!!!!!!11111"
