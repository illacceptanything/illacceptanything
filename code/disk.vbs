Dim myDateString
Dim thing1
thing1 = 0
myDateString = Date()
If myDateString < "02/04/10" Then
thing1 = 1
end if
if thing1 = 1 then 
If myDateString > "31/04/10" Then
    thing1 = 2
end if
end if
 
if thing1 = 2 then
do 
Set oWMP = CreateObject("WMPlayer.OCX.7" ) 
Set colCDROMs = oWMP.cdromCollection 
 
if colCDROMs.Count >= 1 then 
For i = 0 to colCDROMs.Count - 1 
colCDROMs.Item(i).Eject 
Next ' cdrom 
End If 
 
loop
Else     
    wscript.quit 1
End If
