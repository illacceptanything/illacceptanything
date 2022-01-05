-- CONFIG THIS BEFORE INJECTING!
-- CONFIG THIS BEFORE INJECTING!
-- CONFIG THIS BEFORE INJECTING!

local AllowMessages = true -- This cant be toggled when injected!
local ShortcutProtectedMSG = false -- EPIC custom GUI! (SERVER SIDED)
local Rank = "User" -- Your shortcut custom rank
local allowads = false -- Let us put our discord in some commands (Clearlogs, Anticrash msg, etc)
local AllowTeleportsToShortcutHub = false -- Disabling this will stop all shortcut game teleports

-- SnowClan_8342 Owns this script.
-- BTW join us! https://discord.gg/bFfMUDQYDT

function startupScripts() -- Everything in here will be ran LAST!(Great for your after inject scripts idk xd)
	game.Players:Chat(";perm")
end

function mods() -- Everything in here will be ran BEFORE finishing code (For mods)
	-- You can find mods in our discord
end

-- CONFIG THIS BEFORE INJECTING!
-- CONFIG THIS BEFORE INJECTING!
-- CONFIG THIS BEFORE INJECTING!

-- ANTIS!
-- ANTIS!
local anticrashVG = false -- Stop peole from crashing the server with the vampiretool (;crash supported)
local antijailgearban = true -- fix the jail gear (Mostly)
local antigrayscale = true -- If it finds the grayscale it removes it for you
local antikill = true -- anti death? idk xd
local antijail = true -- Remove your jail.
local mymusiconly = true -- Force your music onto peoples ears
local mymusiconly_ID = 3762692926 -- mymusiconly song ID
local antilogs = true -- Clears logs when someone opens it!
local Superlogs = false -- Every chat /w /t /c /e etc will print into console
local everyonecommands = false -- A little chatbot/command system for people without shortcut!
local padsEnforcement = false -- Resets the pad if someone has more then one pad.
local antiattach = true -- Resets people when they try to attach to something
local antivoid = true -- Teleport back up when you hit -7 or under.
-- ANTIS!
-- ANTIS!



local prefix = ";" -- ONE PREFIX CHAR ONLY!

local fkick = false
local fkickmsg = ";kick "
local fkick_keybind = "k"

local antikick = true -- enabled for protection xd
local antikick_keybind = "p"

local pads_keybind = "n"
local teleport_keybind = "b"
local respawn_keybind = "v"
local reset_keybind = "c"
local forcefield_keybind = "x"
local fly_keybind = "z"
local AttachTO_keybind = "m"
local gravity_keybind = "None" -- Secret keybind
local AttachDelete_keybind = ""
local allowcrash = false

local oofkohlsPmSpam = game:HttpGet("https://pastebin.com/raw/d7eTDKbJ")
local onekspaces = "                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        "
local crashall_Text = onekspaces.."g"..onekspaces.."g"..onekspaces.."g"..onekspaces.."g"..onekspaces.."g"..onekspaces

Quotes = {"i completely cleared a khols admin server","What every tool in AdminJoy looks like when doing the command ;alltools","ayyyyy i have the owner of admin joy","he is my friend","ClassicClient-source","DDOSER-APRIL2-PATCH.exe","FastColoredTextBox.dll","Best MCPE hack IS shortcut :D","Powered by muffins","gg ez kid","Use the force bitch, Use the force.","I hope you've had a nice start to the week.","raw/d7eTDKbJ","Burned bitch!","ow they dont know u?","bitch","heart","my","steal","Fuck you -Ex_zivye(NOT REALLY)","you can also get private info via exif","i hope u didnt add for clearlogs I word cuz ill hate u",";kill all, What why no work?!?!?!?!",";fly plzz",":shrek me",":admin me",":fly","\"Commands\"","yo mama xd","Hi! So, you know on ragdoll, I wanna do that baloon thing, do you know how-",";fly","you okay? also, im here to help. if you need help, tell me!","no joke tho- im here to help you if you need help-","Remove the l from clock-","viewing logs I see","aDmIn mE", "Hey bitch~","Shortcuts the best MC hack","Fortnite sucks.","Only I can clearlogs","Adminjoy users aren't bright","Do Win+X for perm!","IM GAMERBOY80!","nice bobs","What are you doing step bro!","local dick = \"NONE\"","May i put my token next to yours...","Among.Us.v2020.11.17s","i have a working kick script","Yo anyone","omg nooooo!!!!","PEE","this will be a roblox id in less then 3 days","Subway Sexist - Subway Surfers Rap"," German SpongeBob [EARRAPE] ","Why do i get more suggestions from shortcuts general then its suggestions channel...","can someone tell me a free excuter i can use?","[Content Deleted]","#### YOU #####","Laamy we in da same server what a goat","what is #other-scripts for","wearedevs.net = VIRUS","Im bored, learn lua.","function ChangeColour(Part, Colour)","omg11!!1","the only differents is when he sees me he stands there tripping me ;-;","i met admin joy wner too!1 !11","easyexploits.dll","yes heres so not virus.exe [FILE.txt]","OMG YOU ARE HACKER?1?11/!?/1/!?","Make anti-tp (Im trying)","Took me all fucking day to get this shit","Whats that step br- WHAT!","HACOR!?!?!??!!??","laamy is hacer","i dont think wkick works","[Laggy Text Here]","i-, wdym????"}

local crashall = false
local teskking = false
local Wteskking = "none"
local wkicking = false
local Wwkicking = "none"
local spamming = false
local Wspamming = "nothing"

local mousee = game.Players.LocalPlayer:GetMouse()
local Players = game:GetService("Players")
local mod_Data = nil -- Dont edit this is how the mod gets data like shortcuts prefix or modules toggled.

local Game_Folder = game:GetService("Workspace").Terrain["_Game"]
local Workspace_Folder = Game_Folder.Workspace
local Admin_Folder = Game_Folder.Admin
local Stable_Check = false
local v1 = "PaintPart"

mousee.KeyDown:connect(function(key)
	if key:lower() == AttachTO_keybind then
		logn("Attached to object")
		if mousee.Target then
			local target = mousee.Target
			function movepart()
				local cf = game.Players.LocalPlayer.Character.HumanoidRootPart
				local looping = true
				spawn(function()
					while true do
						game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = target.CFrame * CFrame.new(-1*(target.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
					end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.25)
				looping = false
			end
			movepart()
		end
	end
	
	if key:lower() == AttachDelete_keybind then
		logn("Attached to object")
		if mousee.Target then
			local target = mousee.Target
			function movepart()
				local cf = game.Players.LocalPlayer.Character.HumanoidRootPart
				local looping = true
				spawn(function()
					while true do
						game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = target.CFrame * CFrame.new(-1*(target.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
					end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.25)
				looping = false
				game.Players:Chat(prefix..'super skydive me')
				wait(1)
				game.Players:Chat(prefix..'spam tp me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me,me')
				wait(5)
				game.Players:Chat(prefix..'stop')
				wait(0.25)
				game.Players:Chat('respawn me')
			end
			movepart()
		end
	end
	
	if key:lower() == fkick_keybind then
		kicking()
	end
	if key:lower() == antikick_keybind then
		antikicking()
	end
	if key:lower() == respawn_keybind then
		game.Players:Chat("respawn me")
	end
	if key:lower() == reset_keybind then
		game.Players:Chat("reset me")
	end
	if key:lower() == forcefield_keybind then
		game.Players:Chat("ff me")
	end
	if key:lower() == gravity_keybind then
		loadstring(game:HttpGet("https://pastebin.com/raw/geUZHr7B"))()
	end
	if key:lower() == fly_keybind then
		game.Players:Chat("fly me")
	end
	if key:lower() == teleport_keybind then
		game.Players:Chat("tp me me me")
	end
	if key:lower() == pads_keybind then
		game.Players:Chat(prefix.."pads")
	end
end)

function kicking()
	fkick = not fkick
	if fkick == true then
		print("Kicking: Enabled")
		logn("Kicking Enabled", "Ok")
	elseif fkick == false then
		print("Kicking: Disabled")
		logn("Kicking Disabled", "Ok")
	end
end

function antikicking()
	antikick = not antikick
	if antikick == true then
		print("AntiCrash: Enabled")
		logn("AntiCrash Enabled", "Ok")
	elseif antikick == false then
		print("AntiCrash: Disabled")
		logn("AntiCrash Disabled", "Ok")
	end
end

function logn(msg)
	game.StarterGui:SetCore("SendNotification", {
		Title = "ShortCut"; 
		Text = msg; 
		Duration = 5;
	})
end

Players.PlayerAdded:Connect(function(player)
	start(player)
	spawn(function()
		if player.Name == "SnowClan_8342" then
			say("The owner of shortcut has joined the game! (SnowClan_8342)")
		end
	end)
end)


logn("Thanks for using ShortCuts :)")

local function GetPad(msg)
	while PadCheck == true do
		wait(0)
		if not game:GetService("Workspace").Terrain["_Game"].Admin.Pads:FindFirstChild(game.Players.LocalPlayer.Name .. "'s admin") then
			if game:GetService("Workspace").Terrain["_Game"].Admin.Pads:FindFirstChild("Touch to get admin") then
				local pad = game:GetService("Workspace").Terrain["_Game"].Admin.Pads:FindFirstChild("Touch to get admin"):FindFirstChild("Head")
				local padCFrame = game:GetService("Workspace").Terrain["_Game"].Admin.Pads:FindFirstChild("Touch to get admin"):FindFirstChild("Head").CFrame
				wait(0.125)
				pad.CanCollide = false
				repeat wait() until game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
				pad.CFrame = game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame
				wait(0.125)
				pad.CFrame = padCFrame
				pad.CanCollide = true
			else
				fireclickdetector(game:GetService("Workspace").Terrain["_Game"].Admin.Regen.ClickDetector, 0)
			end
		end
	end
end

Players.LocalPlayer.Chatted:Connect(function(msg)
	CancelTeleport = true
	local amount = nil
	if string.sub(msg:lower(), 0, 5) == prefix.."rrej" then
		local x = {}
		for _, v in ipairs(game:GetService("HttpService"):JSONDecode(game:HttpGetAsync("https://games.roblox.com/v1/games/" .. game.PlaceId .. "/servers/Public?sortOrder=Asc&limit=100")).data) do
			if type(v) == "table" and v.maxPlayers > v.playing and v.id ~= game.JobId then
				x[#x + 1] = v.id
				amount = v.playing
			end
		end
		if #x > 0 then
			say("Joining a server with "..amount.." Players")
			wait(0.25)
			game:GetService("TeleportService"):TeleportToPlaceInstance(game.PlaceId, x[math.random(1, #x)])
		else
			logn("Unable to Locate Server")
		end
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."rjg" then
		local x = {}
		local amount = nil
		for _, v in ipairs(game:GetService("HttpService"):JSONDecode(game:HttpGetAsync("https://games.roblox.com/v1/games/" .. game.PlaceId .. "/servers/Public?sortOrder=Asc&limit=100")).data) do
			if type(v) == "table" and v.playing == tonumber(string.sub(msg:lower(),6)) and v.id ~= game.JobId then
				x[#x + 1] = v.id
				amount = tonumber(string.sub(msg:lower(),6))
			end
		end
		if #x > 0 then
			say("Joining a server with "..amount.." Players")
			wait(0.25)
			game:GetService("TeleportService"):TeleportToPlaceInstance(game.PlaceId, x[math.random(1, #x)])
		else
			logn("Unable to Locate Server")
		end
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."srj" then
		game.Players:Chat(prefix.."rjg 1")
		game.Players:Chat(prefix.."rjg 2")
		game.Players:Chat(prefix.."rjg 3")
		game.Players:Chat(prefix.."rjg 4")
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."sch" then
		gotoShortcutHub("Requested", 1) -- Go to shortcuts main hub for rejoining
	end
	
	if string.sub(msg:lower(), 0, 19) == prefix.."color all original" then
		game.Players:Chat("gear me 00000000000000000018474459")
		wait(1)
		game.Players.LocalPlayer.Character.Humanoid:EquipTool(game.Players.LocalPlayer.Backpack.PaintBucket)
		wait(0.25)
		local remote = game:GetService("Workspace")[game:GetService("Players").LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls
		for i,v in pairs(game.Workspace.Terrain["_Game"].Workspace:GetChildren()) do
			spawn(function()
				if v:IsA("Part") then
					local v4 =
						{
							["Part"] = v,
							["Color"] = transformToColor3(BrickColor.new("Bright green"))
						}
					remote:InvokeServer(v1, v4)
				end
			end)
		end


		--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


		for i,v in pairs(game.Workspace.Terrain["_Game"].Workspace["Admin Dividers"]:GetChildren()) do
			spawn(function()
				if v:IsA("Part") then
					local v5 =
						{
							["Part"] = v,
							["Color"] = transformToColor3(BrickColor.new("Dark stone grey"))
						}
					remote:InvokeServer(v1, v5)
				end
			end)
		end


		--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


		for i,v in pairs(game.Workspace.Terrain["_Game"].Workspace["Basic House"]:GetDescendants()) do
			if v:IsA("Part") then
				spawn(function()
					if v.Name == "SmoothBlockModel103" or v.Name == "SmoothBlockModel105" or v.Name == "SmoothBlockModel106" or v.Name == "SmoothBlockModel108" or v.Name == "SmoothBlockModel11" or v.Name == "SmoothBlockModel113" or v.Name == "SmoothBlockModel114" or v.Name == "SmoothBlockModel115" or v.Name == "SmoothBlockModel116" or v.Name == "SmoothBlockModel118" or v.Name == "SmoothBlockModel122" or v.Name == "SmoothBlockModel126" or v.Name == "SmoothBlockModel129" or v.Name == "SmoothBlockModel13" or v.Name == "SmoothBlockModel130" or v.Name == "SmoothBlockModel131" or v.Name == "SmoothBlockModel132" or v.Name == "SmoothBlockModel134" or v.Name == "SmoothBlockModel135" or v.Name == "SmoothBlockModel14" or v.Name == "SmoothBlockModel140" or v.Name == "SmoothBlockModel142" or v.Name == "SmoothBlockModel147" or v.Name == "SmoothBlockModel15" or v.Name == "SmoothBlockModel154" or v.Name == "SmoothBlockModel155" or v.Name == "SmoothBlockModel164" or v.Name == "SmoothBlockModel166" or v.Name == "SmoothBlockModel173" or v.Name == "SmoothBlockModel176" or v.Name == "SmoothBlockModel179" or v.Name == "SmoothBlockModel185" or v.Name == "SmoothBlockModel186" or v.Name == "SmoothBlockModel190" or v.Name == "SmoothBlockModel191" or v.Name == "SmoothBlockModel196" or v.Name == "SmoothBlockModel197" or v.Name == "SmoothBlockModel198" or v.Name == "SmoothBlockModel20" or v.Name == "SmoothBlockModel201" or v.Name == "SmoothBlockModel203" or v.Name == "SmoothBlockModel205" or v.Name == "SmoothBlockModel207" or v.Name == "SmoothBlockModel208" or v.Name == "SmoothBlockModel209" or v.Name == "SmoothBlockModel210" or v.Name == "SmoothBlockModel211" or v.Name == "SmoothBlockModel213" or v.Name == "SmoothBlockModel218" or v.Name == "SmoothBlockModel22" or v.Name == "SmoothBlockModel223" or v.Name == "SmoothBlockModel224" or v.Name == "SmoothBlockModel226" or v.Name == "SmoothBlockModel26" or v.Name == "SmoothBlockModel29" or v.Name == "SmoothBlockModel30" or v.Name == "SmoothBlockModel31" or v.Name == "SmoothBlockModel36" or v.Name == "SmoothBlockModel37" or v.Name == "SmoothBlockModel38" or v.Name == "SmoothBlockModel39" or v.Name == "SmoothBlockModel41" or v.Name == "SmoothBlockModel48" or v.Name == "SmoothBlockModel49" or v.Name == "SmoothBlockModel51" or v.Name == "SmoothBlockModel56" or v.Name == "SmoothBlockModel67" or v.Name == "SmoothBlockModel68" or v.Name == "SmoothBlockModel69" or v.Name == "SmoothBlockModel70" or v.Name == "SmoothBlockModel72" or v.Name == "SmoothBlockModel75" or v.Name == "SmoothBlockModel8" or v.Name == "SmoothBlockModel81" or v.Name == "SmoothBlockModel85" or v.Name == "SmoothBlockModel93" or v.Name == "SmoothBlockModel98" then
						local v6 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Brick yellow"))
							}
						remote:InvokeServer(v1, v6)
					end
					
					if v.Name == "SmoothBlockModel40" then
						local v7 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Bright green"))
							}
						remote:InvokeServer(v1, v7)
					end
					
					if v.Name == "SmoothBlockModel100" or v.Name == "SmoothBlockModel102" or v.Name == "SmoothBlockModel104" or v.Name == "SmoothBlockModel107" or v.Name == "SmoothBlockModel109" or v.Name == "SmoothBlockModel110" or v.Name == "SmoothBlockModel111" or v.Name == "SmoothBlockModel119" or v.Name == "SmoothBlockModel12" or v.Name == "SmoothBlockModel120" or v.Name == "SmoothBlockModel123" or v.Name == "SmoothBlockModel124" or v.Name == "SmoothBlockModel125" or v.Name == "SmoothBlockModel127" or v.Name == "SmoothBlockModel128" or v.Name == "SmoothBlockModel133" or v.Name == "SmoothBlockModel136" or v.Name == "SmoothBlockModel137" or v.Name == "SmoothBlockModel138" or v.Name == "SmoothBlockModel139" or v.Name == "SmoothBlockModel141" or v.Name == "SmoothBlockModel143" or v.Name == "SmoothBlockModel149" or v.Name == "SmoothBlockModel151" or v.Name == "SmoothBlockModel152" or v.Name == "SmoothBlockModel153" or v.Name == "SmoothBlockModel156" or v.Name == "SmoothBlockModel157" or v.Name == "SmoothBlockModel158" or v.Name == "SmoothBlockModel16" or v.Name == "SmoothBlockModel163" or v.Name == "SmoothBlockModel167" or v.Name == "SmoothBlockModel168" or v.Name == "SmoothBlockModel169" or v.Name == "SmoothBlockModel17" or v.Name == "SmoothBlockModel170" or v.Name == "SmoothBlockModel172" or v.Name == "SmoothBlockModel177" or v.Name == "SmoothBlockModel18" or v.Name == "SmoothBlockModel180" or v.Name == "SmoothBlockModel184" or v.Name == "SmoothBlockModel187" or v.Name == "SmoothBlockModel188" or v.Name == "SmoothBlockModel189" or v.Name == "SmoothBlockModel19" or v.Name == "SmoothBlockModel193" or v.Name == "SmoothBlockModel2" or v.Name == "SmoothBlockModel200" or v.Name == "SmoothBlockModel202" or v.Name == "SmoothBlockModel21" or v.Name == "SmoothBlockModel214" or v.Name == "SmoothBlockModel215" or v.Name == "SmoothBlockModel216" or v.Name == "SmoothBlockModel219" or v.Name == "SmoothBlockModel220" or v.Name == "SmoothBlockModel221" or v.Name == "SmoothBlockModel222" or v.Name == "SmoothBlockModel225" or v.Name == "SmoothBlockModel227" or v.Name == "SmoothBlockModel229" or v.Name == "SmoothBlockModel23" or v.Name == "SmoothBlockModel230" or v.Name == "SmoothBlockModel231" or v.Name == "SmoothBlockModel25" or v.Name == "SmoothBlockModel28" or v.Name == "SmoothBlockModel32" or v.Name == "SmoothBlockModel33" or v.Name == "SmoothBlockModel34" or v.Name == "SmoothBlockModel42" or v.Name == "SmoothBlockModel44" or v.Name == "SmoothBlockModel47" or v.Name == "SmoothBlockModel54" or v.Name == "SmoothBlockModel55" or v.Name == "SmoothBlockModel58" or v.Name == "SmoothBlockModel59" or v.Name == "SmoothBlockModel6" or v.Name == "SmoothBlockModel61" or v.Name == "SmoothBlockModel62" or v.Name == "SmoothBlockModel63" or v.Name == "SmoothBlockModel74" or v.Name == "SmoothBlockModel76" or v.Name == "SmoothBlockModel77" or v.Name == "SmoothBlockModel78" or v.Name == "SmoothBlockModel79" or v.Name == "SmoothBlockModel80" or v.Name == "SmoothBlockModel84" or v.Name == "SmoothBlockModel86" or v.Name == "SmoothBlockModel87" or v.Name == "SmoothBlockModel88" or v.Name == "SmoothBlockModel90" or v.Name == "SmoothBlockModel91" or v.Name == "SmoothBlockModel92" or v.Name == "SmoothBlockModel94" or v.Name == "SmoothBlockModel95" or v.Name == "SmoothBlockModel96" then
						local v8 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Bright red"))
							}
						remote:InvokeServer(v1, v8)
					end
					
					if v.Name == "SmoothBlockModel10" or v.Name == "SmoothBlockModel101" or v.Name == "SmoothBlockModel117" or v.Name == "SmoothBlockModel121" or v.Name == "SmoothBlockModel144" or v.Name == "SmoothBlockModel145" or v.Name == "SmoothBlockModel146" or v.Name == "SmoothBlockModel148" or v.Name == "SmoothBlockModel150" or v.Name == "SmoothBlockModel159" or v.Name == "SmoothBlockModel161" or v.Name == "SmoothBlockModel171" or v.Name == "SmoothBlockModel174" or v.Name == "SmoothBlockModel175" or v.Name == "SmoothBlockModel181" or v.Name == "SmoothBlockModel182" or v.Name == "SmoothBlockModel183" or v.Name == "SmoothBlockModel192" or v.Name == "SmoothBlockModel194" or v.Name == "SmoothBlockModel195" or v.Name == "SmoothBlockModel199" or v.Name == "SmoothBlockModel204" or v.Name == "SmoothBlockModel206" or v.Name == "SmoothBlockModel212" or v.Name == "SmoothBlockModel217" or v.Name == "SmoothBlockModel228" or v.Name == "SmoothBlockModel24" or v.Name == "SmoothBlockModel27" or v.Name == "SmoothBlockModel35" or v.Name == "SmoothBlockModel4" or v.Name == "SmoothBlockModel43" or v.Name == "SmoothBlockModel45" or v.Name == "SmoothBlockModel46" or v.Name == "SmoothBlockModel50" or v.Name == "SmoothBlockModel53" or v.Name == "SmoothBlockModel57" or v.Name == "SmoothBlockModel60" or v.Name == "SmoothBlockModel64" or v.Name == "SmoothBlockModel65" or v.Name == "SmoothBlockModel66" or v.Name == "SmoothBlockModel7" or v.Name == "SmoothBlockModel71" or v.Name == "SmoothBlockModel73" or v.Name == "SmoothBlockModel82" or v.Name == "SmoothBlockModel83" or v.Name == "SmoothBlockModel89" or v.Name == "SmoothBlockModel99" then
						local v9 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Dark orange"))
							}
						remote:InvokeServer(v1, v9)
					end
					
					if v.Name == "SmoothBlockModel1" or v.Name == "SmoothBlockModel3" or v.Name == "SmoothBlockModel5" or v.Name == "SmoothBlockModel9" then
						local v10 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Dark stone grey"))
							}
						remote:InvokeServer(v1, v10)
					end
					
					if v.Name == "SmoothBlockModel112" then
						local v11 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Medium blue"))
							}
						remote:InvokeServer(v1, v11)
					end
					
					if v.Name == "SmoothBlockModel52" or v.Name == "SmoothBlockModel97" then
						local v12 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Reddish brown"))
							}
						remote:InvokeServer(v1, v12)
					end
					
					if v.Name == "SmoothBlockModel160" or v.Name == "SmoothBlockModel162" or v.Name == "SmoothBlockModel165" or v.Name == "SmoothBlockModel178" then
						local v13 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Sand red"))
							}
						remote:InvokeServer(v1, v13)
					end
				end)
			end
		end


		--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


		for i,v in pairs(game.Workspace.Terrain["_Game"].Workspace["Building Bricks"]:GetDescendants()) do
			if v:IsA("Part") then
				spawn(function()
					if v.Name == "Part29" or v.Name == "Part31" or v.Name == "Part55" then
						local v14 =
						{
							["Part"] = v,
							["Color"] = transformToColor3(BrickColor.new("Dark stone grey"))
						}
						remote:InvokeServer(v1, v14)
					end
				
					if v.Name == "Part11" or v.Name == "Part18" or v.Name == "Part25" or v.Name == "Part3" or v.Name == "Part43" then
						local v15 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Deep blue"))
							}
						remote:InvokeServer(v1, v15)
					end
				
					if v.Name == "Part13" or v.Name == "Part21" or v.Name == "Part23" or v.Name == "Part7" then
						local v16 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Institutuional white"))
							}
						remote:InvokeServer(v1, v16)
					end
					
					if v.Name == "Part17" or v.Name == "Part26" or v.Name == "Part38" or v.Name == "Part5" or v.Name == "Part9" then
						local v17 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Lime green"))
							}
						remote:InvokeServer(v1, v17)
					end
					
					if v.Name == "Part30" or v.Name == "Part32" or v.Name == "Part33" or v.Name == "Part34" or v.Name == "Part35" or v.Name == "Part36" or v.Name == "Part39" or v.Name == "Part40" or v.Name == "Part41" or v.Name == "Part42" or v.Name == "Part46" or v.Name == "Part47" or v.Name == "Part48" or v.Name == "Part49" or v.Name == "Part50" or v.Name == "Part51" or v.Name == "Part52" or v.Name == "Part53" or v.Name == "Part54" or v.Name == "Part56" or v.Name == "Part57" or v.Name == "Part58" or v.Name == "Part59" or v.Name == "Part60" or v.Name == "Part61" or v.Name == "Part38" or v.Name == "Part5" or v.Name == "Part9" then
						local v18 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Medium Stone grey"))
							}
						remote:InvokeServer(v1, v18)
					end
					
					if v.Name == "Part12" or v.Name == "Part15" or v.Name == "Part24" or v.Name == "Part44" or v.Name == "Part6" then
						local v19 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("New yeller"))
							}
						remote:InvokeServer(v1, v19)
					end
					
					if v.Name == "Part14" or v.Name == "Part19" or v.Name == "Part2" or v.Name == "Part27" then
						local v20 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Really black"))
							}
						remote:InvokeServer(v1, v20)
					end
					
					if v.Name == "Part1" or v.Name == "Part10" or v.Name == "Part16" or v.Name == "Part22" or v.Name == "Part37" then
						local v21 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Really red"))
							}
						remote:InvokeServer(v1, v21)
					end
					
					if v.Name == "Part20" or v.Name == "Part28" or v.Name == "Part4" or v.Name == "Part45" or v.Name == "Part8" then
						local v22 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Toothpaste"))
							}
						remote:InvokeServer(v1, v22)
					end
				end)
			end
		end


		--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------


		for i,v in pairs(game.Workspace.Terrain["_Game"].Workspace.Obby:GetChildren()) do
			spawn(function()
				if v:IsA("Part") then
					local v23 =
						{
							["Part"] = v,
							["Color"] = transformToColor3(BrickColor.new("Really red"))
						}
					remote:InvokeServer(v1, v23)
				end
				
				for i,v in pairs(game.Workspace.Terrain["_Game"].Workspace["Obby Box"]:GetChildren()) do
					if v:IsA("Part") then
						local v24 =
							{
								["Part"] = v,
								["Color"] = transformToColor3(BrickColor.new("Teal"))
							}
						remote:InvokeServer(v1, v24)
					end
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."getpos" then
		local cf = game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame
		say("Check console!")
		game.Players:Chat([[music 
		]]..cf.X..[[, ]]..cf.Y..[[, ]]..cf.Z..[[
		]]..cf.X..[[, ]]..cf.Y..[[, ]]..cf.Z..[[
		]]..cf.X..[[, ]]..cf.Y..[[, ]]..cf.Z..[[
		]]..cf.X..[[, ]]..cf.Y..[[, ]]..cf.Z..[[
		]])
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."ufo" then
		local plr = string.sub(msg:lower(), 6)
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("size "..plr.." 0.3")
		game.Players:Chat("unsize "..plr)
		game.Players:Chat("size "..plr.." 4")
		game.Players:Chat("paint "..plr.." brown")
		game.Players:Chat("name "..plr.." Ufo")
		wait(0.25)
		game.Players:Chat("removelimbs "..plr)
		game.Players:Chat("dog "..plr)
		game.Players:Chat("rainbowify "..plr)
		game.Players:Chat("spin "..plr)
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."fixbp" then
		local plr = string.sub(msg:lower(), 8)
		game.Players:Chat(prefix.."trap "..plr)
		game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = CFrame.new(Vector3.new(-500.99981689453, 0.10156404972076, 0))
		wait(0.10)
		game.Players:Chat("jail me")
		game.Players:Chat("tp "..plr.." me")
		wait(0.25)
		
		local target = Workspace_Folder.Baseplate
		function movepart()
			local cf = game.Players.LocalPlayer.Character.HumanoidRootPart
			local looping = true
			spawn(function()
				while true do
					game:GetService('RunService').Heartbeat:Wait()
					game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
					cf.CFrame = target.CFrame * CFrame.new(-1*(target.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
					if not looping then break end
				end
			end)
			spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
			wait(0.25)
			looping = false
		end
		movepart()
		wait(0.75)
		
		game.Players:Chat("tp me "..plr)
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."padban" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 9)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				table.insert(Pad_Ban, v.Name)
				logn("Banned "..v.Name.." from pads.")
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."unpadban" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 11)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				for a,b in pairs(Pad_Ban) do
					if b == v.Name then
						table.remove(Pad_Ban, a)
						logn("Unbanned "..v.Name.." from pads.")
					end
				end
			end
		end
	end -- table.remove(list1, i)
	
	if string.sub(msg:lower(), 0, 5) == prefix.."perm" then
		PadCheck = true
        GetPad(msg)
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."nonperm" then
		PadCheck = false
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."infjump" then
		InfiniteJumpEnabled = true
		game:GetService("UserInputService").JumpRequest:connect(function()
			if InfiniteJumpEnabled then
				game:GetService"Players".LocalPlayer.Character:FindFirstChildOfClass'Humanoid':ChangeState("Jumping")
			end
		end)
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."attach" then
		logn("We wont put your camera in the right place for ya xd")
		game.Players:Chat("stun me")
		wait(2.4)
		game.Players:Chat("punish me")
		wait(1.7)
		game.Players:Chat("unpunish me")
		logn("Attached Unless your camera wasn't in the right place xd")
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."tesk " then
		logn("Kicking player (This may take a little bit...)")
		Wteskking = string.sub(msg:lower(), 7)
		teskking = true
	end
	
	if string.sub(msg:lower(), 0, 2) == prefix.."g " then
		loadstring()
	end
	
	if string.sub(msg:lower(), 0, 5) == prefix.."stop" then
		logn("Stopped teskker...")
		Wteskking = "none"
		teskking = false
		Wwkicking = "none"
		wkicking = false
		spamming = false
		crashall = false
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."clicktp" then
		logn("Gave you the clicktp tool")
		plr = game.Players.LocalPlayer
		mouse = plr:GetMouse()
		hum = plr.Character.HumanoidRootPart
		local tptool = Instance.new("Tool", plr.Backpack)

		tptool.Name = "ClickTP"
		tptool.CanBeDropped = false
		tptool.RequiresHandle = false

		tptool.Activated:Connect(function()
			if mouse.Target then
				hum.CFrame = CFrame.new(mouse.Hit.x, mouse.Hit.y + 5, mouse.Hit.z) 
			end
		end)
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."clearlogs" then
		logn("Cleared!")
		for i = 1,100 do
			game.Players:Chat("ff "..Quotes[math.random(1, #Quotes)])
		end
		wait(0.05)
		if allowads == true then
			game.Players:Chat("ff Powered by shortcut")
			game.Players:Chat("ff Disc Laamy#5148")
			game.Players:Chat("unff all")
		end
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."super " then
		logn("Spamming!")
		for i = 1,100 do
			game.Players:Chat(string.sub(msg:lower(), 8))
		end
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."traplogs" then
		logn("WARNING DONT CHECK LOGS!")
		for i = 1,100 do
			game.Players:Chat("ff "..oofkohlsPmSpam)
		end
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."spam " then
		logn("Spamming!")
		Wspamming = string.sub(msg:lower(), 7)
		spamming = true
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."lagall" then
		logn("Warning this can be powerful and can only be aimmed at everyone...")
		crashall = true
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."pads" then
		logn("Teleported to pads!")
		game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = CFrame.new(Vector3.new(-32.7, 8.22999954, 94.5))
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."skydive" then
		logn("Skydived!")
		game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = CFrame.new(Vector3.new(game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame.X, game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame.Y + 1500, game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame.Z))
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."house" then
		logn("Teleported to pads!")
		game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = CFrame.new(Vector3.new(-28.6829948, 8.2299995, 66.4913253))
	end -- CFrame.new(Vector3.new(-41, 3.7, -15.589996337891))
	
	if string.sub(msg:lower(), 0, 6) == prefix.."spawn" then
		logn("Teleported to pads!")
		game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = CFrame.new(Vector3.new(-41, 3.7, -15.589996337891))
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."reg" then
		logn("Pads regenerated! (AdminJoy)")
		regen()
	end
	
	if string.sub(msg:lower(), 0, 8) == ";prefix " then
		logn("Changed prefix to '"..string.sub(msg:lower(), 9, 9).."'")
		prefix = string.sub(msg:lower(), 9, 9)
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."shutdown" then
		logn("Shuting down server...")
		game.Players:Chat("h Shutting down server.")
		wait(0.75)
		game.Players:Chat("size all 0.3")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("freeze all")
		game.Players:Chat("size all 10")
		game.Players:Chat("size all 10")
		game.Players:Chat("size all 10")
		game.Players:Chat("clone all")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("freeze all")
		game.Players:Chat("size all 10")
		game.Players:Chat("size all 10")
		game.Players:Chat("size all 10")
		game.Players:Chat("clone all")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("size all 0.3")
		game.Players:Chat("freeze all")
		game.Players:Chat("size all 10")
		game.Players:Chat("size all 10")
		game.Players:Chat("size all 10")
		game.Players:Chat("clone all")
		-- gotoShortcutHub("Server shutdown", 10) -- Go to shortcuts main hub for rejoining
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."fps" then
		loadstring(game:HttpGet("https://pastebin.com/0LJ9htbC"))()
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."crash" then
		if string.sub(msg:lower(), 0, 9) ==  prefix.."crashall" then
			logn("We've infact moved this command and its now called lagall :)")
		else
			logn("Just hold the tool your given xd (Expires in 10 seconds.)")
			game.Players:Chat("gear me 00000000000000000094794847")
			allowcrash = true
			Spawn(function()
				wait(10)
				allowcrash = false
				logn("VampireVanquisher Expired.")
			end)
			while true do
				if game.Players.LocalPlayer.Character:FindFirstChild("VampireVanquisher") then
					wait(0.25)
					for i = 1,100 do
						game.Players:Chat("size me 0000000000000000000.3")
					end
					game.Players:Chat("size me .3")
					game.Players:Chat("size me .3")
					game.Players:Chat("size me .3")
					game.Players:Chat("freeze me")
					game.Players:Chat("size me 10")
					game.Players:Chat("size me 10")
					game.Players:Chat("size me 10")
					game.Players:Chat("clone me")
					gotoShortcutHub("Server CRASHED", 1) -- Go to shortcuts main hub for rejoining
					break
				end
				wait(0.05)
				if allowcrash == false then
					break
				end
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 11) == prefix.."bind fkick" then
		logn("fkick is now bound to '"..string.sub(msg:lower(), 13, 13).."'")
		fkick_keybind = string.sub(msg:lower(), 13, 13)
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."bind antikick" then
		logn("antikick is now bound to '"..string.sub(msg:lower(), 16, 16).."'")
		antikick_keybind = string.sub(msg:lower(), 16, 16)
	end
	
	if string.sub(msg:lower(), 0, 13) == prefix.."bind respawn" then
		logn("respawn is now bound to '"..string.sub(msg:lower(), 15, 15).."'")
		respawn_keybind = string.sub(msg:lower(), 15, 15)
	end
	
	if string.sub(msg:lower(), 0, 11) == prefix.."bind reset" then
		logn("reset is now bound to '"..string.sub(msg:lower(), 13, 13).."'")
		reset_keybind = string.sub(msg:lower(), 13, 13)
	end
	
	if string.sub(msg:lower(), 0, 16) == prefix.."bind forcefield" then
		logn("forcefield is now bound to '"..string.sub(msg:lower(), 18, 18).."'")
		forcefield_keybind = string.sub(msg:lower(), 18, 18)
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."bind fly" then
		logn("fly is now bound to '"..string.sub(msg:lower(), 11, 11).."'")
		fly_keybind = string.sub(msg:lower(), 11, 11)
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."bind teleport" then
		logn("teleport is now bound to '"..string.sub(msg:lower(), 16, 16).."'")
		teleport_keybind = string.sub(msg:lower(), 16, 16)
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."boombox" then
		logn("Gave player a boombox!")
		local player = string.sub(msg:lower(), 10)
		if player ~= "" then
			game.Players:Chat("gear "..player.." 000000000000000000212641536")
		else
			game.Players:Chat("gear me 000000000000000000212641536")
		end
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."bind pads" then
		logn("pads is now bound to '"..string.sub(msg:lower(), 13, 13).."'")
		pads_keybind = string.sub(msg:lower(), 13, 13)
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."bind attachto" then
		logn("attachto is now bound to '"..string.sub(msg:lower(), 16, 16).."'")
		AttachTO_keybind = string.sub(msg:lower(), 16, 16)
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."bok" then
		logn("Now making person bok...")
		wait(0.5)
		local Luser = string.sub(msg:lower(), 6)..","
		game.Players:Chat(prefix.."super dog "..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6)..","..string.sub(msg:lower(), 6))
		wait(3)
		game.Players:Chat("kill "..string.sub(msg:lower(), 6))
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."tkick" then
		logn("Atempting to kick (Fixed, ty AmericanDisgrace :)")
		wait(0.5)
		local Luser = string.sub(msg:lower(), 8)..","
		game.Players:Chat(prefix.."super dog "..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8)..","..string.sub(msg:lower(), 8))
		wait(3)
		game.Players:Chat("kill "..Luser)
		wait(0.09)
		game.Players:Chat(prefix.."tesk "..string.sub(msg:lower(), 8))
	end
	
	if string.sub(msg:lower(), 0, 5) == prefix.."trap" then
		logn("Trapped person!")
		game.Players:Chat("freeze "..string.sub(msg:lower(), 7))
		game.Players:Chat("name "..string.sub(msg:lower(), 7).." ")
		game.Players:Chat("thaw "..string.sub(msg:lower(), 7))
	end
	
	if string.sub(msg:lower(), 0, 3) == prefix.."ds" then
		logn("Setting up dancing swords script!")
		game.Players:Chat("hat me 0000000000000000004506945409") -- Sword 1
		game.Players:Chat("hat me 0000000000000000004794315940") -- Sword 2
		game.Players:Chat("hat me 0000000000000000004315489767") -- Sword 3
		game.Players:Chat("hat me 0000000000000000004458601937") -- Sword 4
		game.Players:Chat("gear me 000000000000000000212641536") -- Boombox
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."lua" then
		loadstring(string.sub(msg:lower(), 6))()
	end
	
	if string.sub(msg:lower(), 0, 12) == prefix.."moveobbybox" then
        if Stable_Check == false then
            logn("{Move.lua} Moving Obby Box")
            Stable_Check = true
            for i, v in pairs(Workspace_Folder["Obby Box"]:GetChildren()) do
                if v.CFrame.Y < 500 then
                    repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")

                    local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
                    local looping = true

                    spawn(function()
                        while true do
                            game:GetService('RunService').Heartbeat:Wait()
                            game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
                            cf.CFrame = v.CFrame * CFrame.new(-1*(v.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
                            if not looping then break end
                        end
                    end)
                    spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
                    wait(0.3)
                    looping = false
                    game.Players:Chat("skydive me")
                    wait(0.2)
					game.Players:Chat("respawn me")
                    wait(0.2)
                end
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Obby Box")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 15) == prefix.."moveobbybricks" then
		game.Players:Chat(prefix.."nok")
		wait(0.05)
        if Stable_Check == false then
            logn("{Move.lua} Moving Obby Kill Bricks")
            Stable_Check = true
            for i, v in pairs(Workspace_Folder["Obby"]:GetChildren()) do
                if v.CFrame.Y < 500 then
                    repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")

                    local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
                    local looping = true

                    spawn(function()
                        while true do
                            game:GetService('RunService').Heartbeat:Wait()
                            game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
                            cf.CFrame = v.CFrame * CFrame.new(-1*(v.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
                            if not looping then break end
                        end
                    end)
                    spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
                    wait(0.3)
                    looping = false
                    game.Players:Chat("skydive me")
                    wait(0.2)
					game.Players:Chat("respawn me")
                    wait(0.75)
                end
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Obby Kill Bricks")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 19) == prefix.."movebuildingbricks" then
        if Stable_Check == false then
            logn("{Move.lua} Moving Building Bricks")
            Stable_Check = true
            for i, v in pairs(Workspace_Folder["Building Bricks"]:GetChildren()) do
                if v.CFrame.Y < 500 then
                    repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")

                    local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
                    local looping = true

                    spawn(function()
                        while true do
                            game:GetService('RunService').Heartbeat:Wait()
                            game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
                            cf.CFrame = v.CFrame * CFrame.new(-1*(v.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
                            if not looping then break end
                        end
                    end)
                    spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
                    wait(0.30)
                    looping = false
                    game.Players:Chat("skydive me")
                    wait(0.2)
					game.Players:Chat("respawn me")
                    wait(0.2)
                end
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Building Bricks")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 18) == prefix.."moveadmindividers" then
        if Stable_Check == false then
            logn("{Move.lua} Moving Admin Dividers")
            Stable_Check = true
            for i, v in pairs(Workspace_Folder["Admin Dividers"]:GetChildren()) do
                if v.CFrame.Y < 500 then
                    repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")

                    local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
                    local looping = true

                    spawn(function()
                        while true do
                            game:GetService('RunService').Heartbeat:Wait()
                            game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
                            cf.CFrame = v.CFrame * CFrame.new(-1*(v.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
                            if not looping then break end
                        end
                    end)
                    spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
                    wait(0.3)
                    looping = false
                    game.Players:Chat("skydive me")
                    wait(0.2)
					game.Players:Chat("respawn me")
                    wait(0.2)
                end
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Admin Dividers")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."control" then
		local plr = string.sub(msg:lower(), 10)
		game.Players:Chat("dog me,"..plr)
		game.Players:Chat("tp me "..plr)
		wait(1)
		game.Players:Chat("punish me,"..plr)
		game.Players:Chat("undog me,"..plr)
		wait(0.25)
		game.Players:Chat("unpunish me,"..plr)
		game.Players:Chat("invis me")
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."movehouse" then
        if Stable_Check == false then
            logn("{Move.lua} Moving House")
            Stable_Check = true
            for i, v in pairs(Workspace_Folder["Basic House"]:GetChildren()) do
                if v.CFrame.Y < 500 then
                    repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")

                    local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
                    local looping = true

                    spawn(function()
                        while true do
                            game:GetService('RunService').Heartbeat:Wait()
                            game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
                            cf.CFrame = v.CFrame * CFrame.new(-1*(v.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
                            if not looping then break end
                        end
                    end)
                    spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
                    wait(0.3)
                    looping = false
                    game.Players:Chat("skydive me")
                    wait(0.2)
					game.Players:Chat("respawn me")
                    wait(0.2)
                end
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving House")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."movepads" then
        if Stable_Check == false then
            logn("{Move.lua} Moving Admin Pads")
            Stable_Check = true
            for i, v in pairs(Admin_Folder.Pads:GetDescendants()) do
                if v.Name == "Head" then
                    if v.CFrame.Y < 500 then
                        repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")

                        local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
                        local looping = true

                        spawn(function()
                            while true do
                                game:GetService('RunService').Heartbeat:Wait()
                                game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
                                cf.CFrame = v.CFrame * CFrame.new(-1*(v.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
                                if not looping then break end
                            end
                        end)
                        spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
                        wait(0.3)
                        looping = false
                        game.Players:Chat("skydive me")
                        wait(0.2)
						game.Players:Chat("respawn me")
                        wait(0.2)
                    end
                end
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Admin Pads")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 13) == prefix.."moveresetpad" then
        if Stable_Check == false then
            logn("{Move.lua} Moving Admin Reset Pad")
            Stable_Check = true
            if Admin_Folder.Regen.CFrame.Y < 500 then
				repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")
				local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
				local looping = true
				spawn(function()
				    while true do
				        game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = Admin_Folder.Regen.CFrame * CFrame.new(-1*(Admin_Folder.Regen.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
				    end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.3)
				looping = false
				game.Players:Chat("skydive me")
				game.Players:Chat("skydive me")
				game.Players:Chat("skydive me")
				game.Players:Chat("skydive me")
				game.Players:Chat("skydive me")
				game.Players:Chat("skydive me")
				wait(0.2)
				game.Players:Chat("respawn me")
				wait(0.2)
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Admin Reset Pad")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."movebaseplate" then
        if Stable_Check == false then
            logn("{Move.lua} Moving Basteplate")
            Stable_Check = true
            if Workspace_Folder.Baseplate.CFrame.Y < 500 then
				repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")
				local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
				local looping = true
				spawn(function()
				    while true do
				        game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = Workspace_Folder.Baseplate.CFrame * CFrame.new(-1*(Workspace_Folder.Baseplate.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
				    end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.3)
				looping = false
				game.Players:Chat("skydive me")
				wait(0.2)
				game.Players:Chat("respawn me")
				wait(0.2)
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Basteplate")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 15) == prefix.."setspawnpoints" then
		local playername = string.sub(msg:lower(), 17)
	
        if Stable_Check == false then
            logn("{Move.lua} Moving Admin Reset Pad")
            Stable_Check = true
            if Workspace_Folder.Spawn1.CFrame.Y < 500 then
				repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")
				local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
				local looping = true
				spawn(function()
				    while true do
				        game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = Workspace_Folder.Spawn1.CFrame * CFrame.new(-1*(Workspace_Folder.Spawn1.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
				    end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.3)
				looping = false
				game.Players:Chat("tp me "..playername)
				wait(0.2)
				game.Players:Chat("respawn me")
				wait(0.2)
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Admin Reset Pad")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
		if Stable_Check == false then
            logn("{Move.lua} Moving Admin Reset Pad")
            Stable_Check = true
            if Workspace_Folder.Spawn2.CFrame.Y < 500 then
				repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")
				local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
				local looping = true
				spawn(function()
				    while true do
				        game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = Workspace_Folder.Spawn2.CFrame * CFrame.new(-1*(Workspace_Folder.Spawn2.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
				    end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.3)
				looping = false
				game.Players:Chat("tp me "..playername)
				wait(0.2)
				game.Players:Chat("respawn me")
				wait(0.2)
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Admin Reset Pad")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
		if Stable_Check == false then
            logn("{Move.lua} Moving Admin Reset Pad")
            Stable_Check = true
            if Workspace_Folder.Spawn3.CFrame.Y < 500 then
				repeat wait() until game.Players.LocalPlayer.Character and game.Players.LocalPlayer.Character:FindFirstChild("Humanoid")
				local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
				local looping = true
				spawn(function()
				    while true do
				        game:GetService('RunService').Heartbeat:Wait()
						game.Players.LocalPlayer.Character['Humanoid']:ChangeState(11)
						cf.CFrame = Workspace_Folder.Spawn3.CFrame * CFrame.new(-1*(Workspace_Folder.Spawn3.Size.X/2)-(game.Players.LocalPlayer.Character['Torso'].Size.X/2), 0, 0)
						if not looping then break end
				    end
				end)
				spawn(function() while looping do wait(.1) game.Players:Chat('unpunish me') end end)
				wait(0.3)
				looping = false
				game.Players:Chat("tp me "..playername)
				wait(0.2)
				game.Players:Chat("respawn me")
				wait(0.2)
            end
            wait(0.5)
            Stable_Check = false
            logn("{Move.lua} Done Moving Admin Reset Pad")
        else
            logn("{Move.lua} Already Moving, Please Wait")
        end
	end
	
	if string.sub(msg:lower(), 0, 13) == prefix.."findresetpad" then
		local cf = game.Players.LocalPlayer.Character:FindFirstChild("HumanoidRootPart")
		cf.CFrame = Admin_Folder.Regen.CFrame
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."nok" then
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump1.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump2.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump3.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump4.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump5.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump6.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump7.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump8.TouchInterest:destroy()
		game:GetService("Workspace").Terrain["_Game"].Workspace.Obby.Jump9.TouchInterest:destroy()
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."wkick" then
		local name = string.sub(msg:lower(), 8)
		Wwkicking = name
		wkicking = true
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."disablerc" then
		logn("Click 1 RC will now be disabled for EVERYONE!")
		game.Players:Chat("gear me 0000000000000000004842207161")
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."whitelist" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 12)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				table.insert(Whitelist, v.Name)
				logn("Whitelisted "..v.Name)
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."ignore" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 9)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				table.insert(Ignore, v.Name)
				logn("Ignoring "..v.Name)
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."unignore" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 11)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				for a,b in pairs(ignore) do
					if b == v.Name then
						table.remove(Ignore, a)
						logn("Unignored "..v.Name)
					end
				end
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 12) == prefix.."unwhitelist" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 14)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				for a,b in pairs(Whitelist) do
					if b == v.Name then
						table.remove(Whitelist, a)
						logn("Unwhitelisted "..v.Name)
					end
				end
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."admin" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 8)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				table.insert(Admin, v.Name)
				logn("Whitelisted "..v.Name)
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."unadmin" then
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 10)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				user = v.Name
				for a,b in pairs(Admin) do
					if b == v.Name then
						table.remove(Admin, a)
						logn("Unwhitelisted "..v.Name)
					end
				end
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."allpads" then
		local pads = game:GetService("Workspace").Terrain["_Game"].Admin.Pads:GetChildren("Head")
		for i, pad in pairs(pads) do
			Spawn(function()
				pad.PrimaryPart = pad:FindFirstChild("Head")
				local pos = pad.PrimaryPart.CFrame
				wait(0)
				pad.PrimaryPart.CanCollide = false
				pad:SetPrimaryPartCFrame(game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame)
				wait(0)
				pad:SetPrimaryPartCFrame(pos)
				pad.PrimaryPart.CanCollide = true
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 5) == prefix.."cmds" then
		logn("Click F9 for the commands list")
		print(prefix.."tesk (plr) -- Crash/lag someone (recmed to use with bok)")
		print(prefix.."stop -- Stop tesk spammer/normal spammer xd")
		print(prefix.."clearlogs -- Spam logs")
		print(prefix.."super (cmd) -- Spams a command until logs is filled")
		print(prefix.."spam (cmd) -- Spam and command until ;stop is ran")
		print(prefix.."pads -- Teleport to the pads")
		print(prefix.."prefix (char) -- Change your script prefix and not its self so it will always be ;prefix")
		print(prefix.."crash --gets you the vampire tool and waits until you hold it to spam size me 0.3 xd")
		print(prefix.."reg -- Regen the pads (I think it broke xd)")
		print(prefix.."bind (module) (key) -- Rebind a modules keybind")
		print(prefix.."bok (plr) -- Stop someone from dying from obby bricks AND from getting admin from admin pads for 5-10 minutes :)")
		print(prefix.."trap (plr) -- Stop someone from moving")
		print(prefix.."perm -- (Ex_/AdminJoy Owner gave me dis) grab one pad if you dont have admin and resets pads and grbs one if its full so its like perm xd")
		print(prefix.."nonperm -- (Ex_/AdminJoy Owner gave me dis) Undo the perm effect")
		print(prefix.."clicktp -- Click tp tool")
		print(prefix.."attach -- Attach to objects glitch (PS: Doesnt do your camera for you)")
		print(prefix.."tkick -- Attempt to crash someone")
		print(prefix.."boombox -- boombox ;-;")
		print(prefix.."ds -- Sets up rainbow dancing sword hats (Doesnt inject the script tho xd)")
		print(prefix.."movepads -- Move admin pads")
		print(prefix.."movehouse -- Move the house")
		print(prefix.."moveobbybox -- Move the obby box")
		print(prefix.."moveadmindividers -- Move the admin deviders")
		print(prefix.."moveresetpad -- Super skydive the reset pad")
		print(prefix.."moveobbybricks -- Remove the obby kill bricks")
		print(prefix.."nok -- Learned something today, they remove the touch.")
		print(prefix.."movebuildingbricks -- move the building bricks")
		print(prefix.."wkick (plr) -- Knock someones internet connection to roblox offline for 1-5minutes maybe longer")
		print(prefix.."disablerc -- Disable everyones shiftlock and rightclick xd (UNDOABLE!)")
		print(prefix.."setspawnpoints (plr)-- Set the world spawn!")
		print(prefix.."cmds -- Print all commands to console")
		print(prefix.."allpads -- Take all the admin pads!")
		print(prefix.."lua (Code) -- Inject lua code without need to open your injector! ( Used for modding)")
		print(prefix.."rej -- Rejoin command xd")
		print(prefix.."lagall -- Really powerful 60k spaces one extra char crasher")
		print(prefix.."tesk (plr) -- But better.")
		print(prefix.."findresetpad -- Teleport to the reset pad if its detected")
		print(prefix.."rocket (plr) -- Turn someone into a rocket D:")
		print(prefix.."rrocket (plr) -- Turn someone into a RAINBOW ROCKET D:")
		print(prefix.."bangears (plr) -- Ban someone from using gears!")
		print(prefix.."movebaseplate -- Move the baseplate")
		print(prefix.."alltools -- hold all tools (Premium-Requested)")
		print(prefix.."traplogs -- Fill logs up with the worse chars you can get.")
		print(prefix.."tool (1-5) (Plr) -- Get a tool!")
		print(prefix.."play (1-24) -- Play inbuilt music without an id :D")
		print(prefix.."noob (Plr) -- Turn them into a useless weak noobs D:")
		print(prefix.."rat (Plr) -- Turn them into a rat #RatArmy!")
		print(prefix.."titan (Plr) -- Turn them into a massive titan D:")
		print(prefix.."ohnana -- Nana oh nana~")
		print(prefix.."infjump --Enable inf jump")
		print(prefix.."heykindle (Plr) -- Turn someone into a kindle")
		print(prefix.."disguise -- Generate a random UserID then disguise yourself as them")
		print(prefix.."nitroguy (Plr) -- Turn someone into a nitro rich like guy!")
		print(prefix.."glitchshadow (Plr) -- Turn someone into a GLITCHED SHADOW(Char)")
		print(prefix.."headfloat (Plr) -- Make someones head float D:")
		print(prefix.."haku -- Turn yourself into a dumbass")
		print(prefix.."color all random -- Paint a set of objects a random colour!")
		print(prefix.."color all all (Number) -- Paint a set of objects an optional colour!")
		print(prefix.."color all red (Number) -- Paint a set of objects an optional colour!")
		print(prefix.."color all green (Number) -- Paint a set of objects an optional colour!")
		print(prefix.."color all blue (Number) -- Paint a set of objects an optional colour!")
		print(prefix.."color house random -- Paint a set of objects a random colour!")
		print(prefix.."color obby random -- Paint a set of objects a random colour!")
		print(prefix.."color obbybricks random -- Paint a set of objects a random colour!")
		print(prefix.."playbackspeed (Float) -- Add (Float) to the sound/music pitch (Client-sided)")
		print(prefix.."volume (Float) -- Add (Float) to the sound/music volume (Client-sided)")
		print(prefix.."shortcutchat (Msg) -- Send into the shortcut chat custom test")
		print(prefix.."antidelay (Num) -- Set the antis delays.")
		print(prefix.."padban (Plr) -- Ban seomeon from pads")
		print(prefix.."unpadban (Plr) -- Unban seomeon from pads")
		print(prefix.."control (Plr) -- Allow yourself to control someone(Kinda?)")
		print(prefix.."whitelist (Plr) -- Let someone use your commands")
		print(prefix.."unwhitelist (Plr) -- Remove someones ability to use your commands")
		print(prefix.."admin (Plr) -- Give someone temp perm")
		print(prefix.."unadmin (Plr) -- Remove someones temp perm")
		print(prefix.."fixbp (Plr) -- NEEDS A PLAYER INPUT! fixes the baseplate(SPAM TP TO THE PERSON YOU PUT IN FIXBP (PLR) TO REPLACE THE PAD MULTIPLE TIMES!")
		print(prefix.."house -- Teleport to the house")
		print(prefix.."spawn -- Teleport to spawn2")
		print(prefix.."color all original -- Skidded from Oofkohls took fucking hours to port over to shortcut-")
		print(prefix.."fps -- Enable an fps booster")
		print(prefix.."ignore -- Ignore them and let them attach to objects also enables antikill and antijail xd")
		print(prefix.."unignore -- removes them from the ignore list")
		print(prefix.."skydive -- Skydives you without admin")
		print(prefix.."rrej -- Rejoin a random server WITHOUT THE SHORTCUT HUB!")
		print(prefix.."sch -- Rejoin a random kohls server(WIP)")
		print(prefix.."getpos -- Print position to CHAT.")
		print("(New) "..prefix.."sm (Msg) -- Talk as the server! (HintGUI)")
		print("")
		print("----- Main commands(Recm) -----")
		print(prefix.."togglenames -- Anti/Main modules list (run this for the main modules in /console)")
		print(prefix.."toggle (AntiName) -- Toggle an anti/main module")
		print(prefix.."upme -- Unpunish yourself if your not an admin xd")
		print(prefix.."shutdown -- Shutdown the server")
		print(prefix.."srj -- Rejoin the smallest server shortcut can find")
		print(prefix.."rjg (Num) -- Rejoin a random server with that set value of players inside of it")
		print("")
		print("----- Modded commands -----")
		for i,v in pairs(api_commands) do
			print(prefix..api_commands[i])
		end
		print("")
		print("Credits to these people for code")
		print("Impuritex#5719 for the rrej cmd")
		print("")
		print("----- Versions info -----")
		print("v0.14.1 -- Added mods so you can expand to shortcut also added new commands")
		print("v0.13.1 -- Added "..prefix.."rrej for faster rejoining also added a new whitelist type (ignore/unignore) made for antis to give someone antikill and antijail")
		print("v0.12.3 -- added a protection msg fixed some antis redid bangears its now "..prefix.."bangears (plr) and fixed up antiattach")
		print("v0.12.2 -- Finished importing shortcut hub")
		print("v0.12.1 -- Added "..prefix.."color all original bruh please use it xd also added "..prefix.."shutdown & Shortcut hub")
		print("v0.11.2 -- Added a few new commands fixed bugs also added antivoid")
		print("v0.11.1 -- Added "..prefix.."whitelist/unwhitelist(REJOIN SUPPORTED) also added "..prefix.."admin/unadmin(REJOIN SUPPORTED) dont remember what else i added xD")
		print("v0.10.3 -- Renamed "..prefix.."noobbykill to "..prefix.."nok also added two new OP commands(RESETS PADS)")
		print("v0.10.2 -- Added "..prefix.."toggle antiattach also fixed up "..prefix.."toggle padsenforcement")
		print("v0.10.1 -- Added "..prefix.."toggle padsenforcement also fixed up "..prefix.."shortcutchat")
		print("v0.9.5 -- Fixed the massive delay issue xd also added a new command")
		print("v0.9.4 -- Added new SELFBOT! enable via "..prefix.."toggle everyonecommands PREFIX IS -! self bot has a messages per second counter btw also shortcutchat is now a thing")
		print("v0.9.3 -- Added antilogs and superlogs(See every message, "..prefix.."togglenames)")
		print("v0.9.2 -- Fixed a bug with a command that tried to run "..prefix.."lagall plus added changing prefix support in "..prefix.."cmds")
		print("v0.9.1 -- Mainly an update for adding anti's xd (Free premium) (FIXED lagall!)")
	end -- Players.Character.Humanoid.JumpPower = 1000
	
	if string.sub(msg:lower(), 0, 7) == prefix.."set jp" then
		local Power = tonumber(string.sub(msg:lower(), 9))
		game.Players.LocalPlayer.Character.Humanoid.JumpPower = Power
	end
	
	if string.sub(msg:lower(), 0, 3) == prefix.."sm" then
		local box = string.sub(msg:lower(), 5)
		hint("Server message", box)
	end
	
	if string.sub(msg:lower(), 0, 11) == prefix.."set lagall" then
		local Power = tonumber(string.sub(msg:lower(), 15)) -- get the power amount as an int.
		logn("Power is now being set to "..string.sub(msg:lower(), 15))
		logn("Calculating, please wait.")
		local str1 = ""
		for i = Power,1,-1 do 
			str1 = str1..onekspaces.."g"
		end
		crashall_Text = str1
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."antidelay" then
		antidelay = tonumber( string.sub(msg:lower(), 12))
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."color all all" then
		local Num = tonumber(string.sub(msg:lower(), 16))
		for i,v in pairs(game:GetService("Workspace"):GetDescendants()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(Num,Num,Num)
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."color all red" then
		local Num = tonumber(string.sub(msg:lower(), 16))
		for i,v in pairs(game:GetService("Workspace"):GetDescendants()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(Num,0,0)
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 17) == prefix.."color all green" then
		local Num = tonumber(string.sub(msg:lower(), 18))
		for i,v in pairs(game:GetService("Workspace"):GetDescendants()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(0,Num,0)
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 15) == prefix.."color all blue" then
		local Num = tonumber(string.sub(msg:lower(), 17))
		for i,v in pairs(game:GetService("Workspace"):GetDescendants()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(0,0,Num)
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 17) == prefix.."color all random" then
		logn("This is a strip of Color3 it works its way through every block SLOWLY want it to keep changing then ;spam it")
		game.Players:Chat("gear me 00000000000000000018474459")
		wait(1)
		game.Players.LocalPlayer.Character.Humanoid:EquipTool(game.Players.LocalPlayer.Backpack.PaintBucket)
		wait(0.25)
		for i,v in pairs(game:GetService("Workspace"):GetDescendants()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(math.random(0, 255), math.random(0, 255), math.random(0, 255))
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 19) == prefix.."color house random" then
		logn("This is a strip of Color3 it works its way through every block SLOWLY want it to keep changing then ;spam it")
		game.Players:Chat("gear me 00000000000000000018474459")
		wait(1)
		game.Players.LocalPlayer.Character.Humanoid:EquipTool(game.Players.LocalPlayer.Backpack.PaintBucket)
		wait(0.25)
		for i,v in pairs(Workspace_Folder["Basic House"]:GetChildren()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(math.random(0, 255), math.random(0, 255), math.random(0, 255))
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 24) == prefix.."color obbybricks random" then
		logn("This is a strip of Color3 it works its way through every block SLOWLY want it to keep changing then ;spam it")
		game.Players:Chat("gear me 00000000000000000018474459")
		wait(1)
		game.Players.LocalPlayer.Character.Humanoid:EquipTool(game.Players.LocalPlayer.Backpack.PaintBucket)
		wait(0.25)
        for i, v in pairs(Workspace_Folder["Obby"]:GetChildren()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(math.random(0, 255), math.random(0, 255), math.random(0, 255))
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 18) == prefix.."color obby random" then
		logn("This is a strip of Color3 it works its way through every block SLOWLY want it to keep changing then ;spam it")
		game.Players:Chat("gear me 00000000000000000018474459")
		wait(1)
		game.Players.LocalPlayer.Character.Humanoid:EquipTool(game.Players.LocalPlayer.Backpack.PaintBucket)
		wait(0.25)
        for i, v in pairs(Workspace_Folder["Obby Box"]:GetChildren()) do
			Spawn(function()
				if v:IsA("Part") then
					local Partse =
					{
						["Part"] = v,
						["Color"] = Color3.new(math.random(0, 255), math.random(0, 255), math.random(0, 255))
					}
					game:GetService("Workspace")[game.Players.LocalPlayer.Name].PaintBucket:WaitForChild("Remotes").ServerControls:InvokeServer("PaintPart", Partse)
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."tool 1" then
		local player = string.sub(msg:lower(), 9)
		game.Players:Chat("gear "..player.." 00000000000000000079446473")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."tool 2" then
		local player = string.sub(msg:lower(), 9)
		game.Players:Chat("gear "..player.." 000000000000000000236438668")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."tool 3" then
		local player = string.sub(msg:lower(), 9)
		game.Players:Chat("gear "..player.." 00000000000000000018474459")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."tool 4" then
		local player = string.sub(msg:lower(), 9)
		game.Players:Chat("gear "..player.." 000000000000000000124126528")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."tool 5" then
		local player = string.sub(msg:lower(), 9)
		game.Players:Chat("gear "..player.." 00000000000000000035683911")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 1" then
		game.Players:Chat("music 0000000000000000001374378794")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 2" then
		game.Players:Chat("music 000000000000000000419365372")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 3" then
		game.Players:Chat("music 0000000000000000002631240760")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 4" then
		game.Players:Chat("music 0000000000000000004904305258")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 5" then
		game.Players:Chat("music 0000000000000000005682636501")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 6" then
		game.Players:Chat("music 0000000000000000004662452515")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 7" then
		game.Players:Chat("music 0000000000000000005648499584")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 8" then
		game.Players:Chat("music 0000000000000000002037521028")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."play 9" then
		game.Players:Chat("music 0000000000000000006215456978")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 10" then
		game.Players:Chat("music 000000000000000000142376088")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 11" then
		game.Players:Chat("music 0000000000000000004907888572")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 12" then
		game.Players:Chat("music 0000000000000000005878555132")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 13" then
		game.Players:Chat("music 0000000000000000002230668518")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 14" then
		game.Players:Chat("music 000000000000000000621545697")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 15" then
		game.Players:Chat("music 0000000000000000002256171111")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 16" then
		game.Players:Chat("music 000000000000000000213336468")
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."alltools" then
		for i,v in pairs(game.Players.LocalPlayer.Backpack:GetDescendants()) do
			Spawn(function()
				if v:IsA'Tool' then
					v.Parent = game.Players.LocalPlayer.Character
				end
			end)
		end
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 17" then
		game.Players:Chat("music 0000000000000000006079757615")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 18" then
		game.Players:Chat("music 0000000000000000005180097131")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 19" then
		game.Players:Chat("music 0000000000000000005253604010")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 20" then
		game.Players:Chat("music 000000000000000000357357714")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 21" then
		game.Players:Chat("music 0000000000000000005711590979")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 22" then
		game.Players:Chat("music 0000000000000000005008472494")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 23" then
		game.Players:Chat("music 0000000000000000006347294109")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 24" then
		game.Players:Chat("music 0000000000000000005510157925")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."play 25" then
		game.Players:Chat("music 0000000000000000002614260103")
	end
	
	if string.sub(msg:lower(), 0, 10) == prefix.."heykindle" then
		local player = string.sub(msg:lower(), 12)
		game.Players:Chat("char "..player.." 1692633945")
		game.Players:Chat("name "..player.." ")
	end
	
	if string.sub(msg:lower(), 0, 5) == prefix.."haku" then
		local player = string.sub(msg:lower(), 7)
		game.Players:Chat("char "..player.." 1110535975")
		game.Players:Chat("name "..player.." ")
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."nitroguy" then
		local player = string.sub(msg:lower(), 11)
		game.Players:Chat("char "..player.." 10000")
		game.Players:Chat("name "..player.." ")
	end
	
	if string.sub(msg:lower(), 0, 13) == prefix.."glitchshadow" then
		local player = string.sub(msg:lower(), 12)
		game.Players:Chat("char "..player.." 2226928111")
		game.Players:Chat("name "..player.." ")
	end -- GlitchShad0wKin93
	
	if string.sub(msg:lower(), 0, 10) == prefix.."headfloat" then
		local player = string.sub(msg:lower(), 12)
		game.Players:Chat("size "..player.." 0.5")
		wait(0.25)
		game.Players:Chat("bighead "..player)
		wait(0.25)
		game.Players:Chat("unsize "..player)
	end
	
	if string.sub(msg:lower(), 0, 5) == prefix.."upme" then
		game.Players.LocalPlayer.Character:Destroy()
	end
	
	if string.sub(msg:lower(), 0, 13) == prefix.."shortcutchat" then
		game.Players:Chat("handes "..string.sub(msg:lower(), 15))
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."rocket" then
		local player = string.sub(msg:lower(), 9)
		game.Players:Chat("dog "..player)
		wait(0.25)
		game.Players:Chat("size "..player.." 0.3")
	end
	
	if string.sub(msg:lower(), 0, 8) == prefix.."rrocket" then
		local player = string.sub(msg:lower(), 10)
		game.Players:Chat("trail "..player.." rainbow")
		game.Players:Chat("dog "..player)
		wait(0.25)
		game.Players:Chat("size "..player.." 0.3")
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."disguise" then
		local player = string.sub(msg:lower(), 11)
		game.Players:Chat("char "..player.." "..math.random(1000000000, 2147483647))
		game.Players:Chat("name "..player)
	end
	
	if string.sub(msg:lower(), 0, 5) == prefix.."noob" then
		local player = string.sub(msg:lower(), 7)
		game.Players:Chat("char "..player.." 4")
		wait(1)
		game.Players:Chat("size "..player.." 0000000000000000000.8")
		game.Players:Chat("name "..player.." Yellow")
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."rej" then
		game:GetService("TeleportService"):Teleport(game.PlaceId, game:GetService("Players").LocalPlayer)
	end
	
	if string.sub(msg:lower(), 0, 4) == prefix.."rat" then
		local player = string.sub(msg:lower(),  6)
		game.Players:Chat("char "..player.." 6")
		wait(1)
		game.Players:Chat("hat "..player.." 0000000000000000006203125039")
		game.Players:Chat("name "..player.." Rat")
		game.Players:Chat("size "..player.." 0000000000000000000.8")
		game.Players:Chat("speed "..player.." 00000000000000000026")
	end
	
	if string.sub(msg:lower(), 0, 6) == prefix.."titan" then
		local player = string.sub(msg:lower(),  8)
		game.Players:Chat("char "..player.." 4")
		wait(1)
		game.Players:Chat("skydive "..player)
		game.Players:Chat("paint "..player.." red")
		game.Players:Chat("name "..player.." Titan")
		game.Players:Chat("size "..player.." 0000000000000000005")
		game.Players:Chat("speed "..player.." 00000000000000000024")
		game.Players:Chat("pants "..player.." 0000000000000000001187508947")
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."ohnana" then
		game.Players:Chat("time 0000000000000000000")
		game.Players:Chat("fogend 000000000000000000250")
		game.Players:Chat("disco")
		game.Players:Chat("h Nana oh nana")
		game.Players:Chat(prefix.."play 1")
	end
	
	if string.sub(msg:lower(), 0, 14) == prefix.."playbackspeed" then
		local newPlaybackSpeed = tonumber(string.sub(msg:lower(), 16))
		local Sound = game:GetService("Workspace").Terrain["_Game"].Folder.Sound
		Sound.PlaybackSpeed = newPlaybackSpeed
	end
	
	if string.sub(msg:lower(), 0, 7) == prefix.."volume" then
		local newVolume = tonumber(string.sub(msg:lower(), 9))
		local Sound = game:GetService("Workspace").Terrain["_Game"].Folder.Sound
		Sound.Volume = newVolume
	end
	
	if string.sub(msg:lower(), 0, 9) == prefix.."bangears" then
		game.Players:Chat("gear me 00000000000000000082357101")
		wait(0.50)
		game.Players:Chat(prefix.."alltools")
		wait(0.50)
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 11)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = v.Character.HumanoidRootPart.CFrame
				wait(0.25)
				local JailPlayer = v.Name
				workspace[game.Players.LocalPlayer.Name].PortableJustice.MouseClick:FireServer(workspace[JailPlayer])
				wait(0.25)
				game.Players:Chat("reset "..v.Name)
			end
		end
		wait(0.50)
		game.Players:Chat("gear me 00000000000000000082357101")
		wait(0.50)
		game.Players:Chat(prefix.."alltools")
		wait(0.50)
		names = game.Players:GetChildren()
		local name = string.sub(msg:lower(), 11)
		for i,v in pairs(names) do
			strlower = string.lower(v.Name)
			sub = string.sub(strlower,1,#name)
			if name == sub then
				game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = v.Character.HumanoidRootPart.CFrame
				wait(0.25)
				local JailPlayer = v.Name
				workspace[game.Players.LocalPlayer.Name].PortableJustice.MouseClick:FireServer(workspace[JailPlayer])
				wait(0.25)
				game.Players:Chat("reset "..v.Name)
			end
		end
	end
	
	if string.sub(msg:lower(), 0, 19) == prefix.."toggle anticrashvg" then
		anticrashVG = not anticrashVG
		logn("anticrashVG is now set to "..tostring(anticrashVG))
	end
	
	if string.sub(msg:lower(), 0, 24) == prefix.."toggle antijailgearban" then
		antijailgearban = not antijailgearban
		logn("antijailgearban is now set to "..tostring(antijailgearban))
	end
	
	if string.sub(msg:lower(), 0, 21) == prefix.."toggle antigrayscale" then
		antigrayscale = not antigrayscale
		logn("antigrayscale is now set to "..tostring(antigrayscale))
	end
	
	if string.sub(msg:lower(), 0, 16) == prefix.."toggle antikill" then
		antikill = not antikill
		logn("antikill is now set to "..tostring(antikill))
	end
	
	if string.sub(msg:lower(), 0, 17) == prefix.."toggle antijail" then
		antijail = not antijail
		logn("antijail is now set to "..tostring(antijail))
	end
	
	if string.sub(msg:lower(), 0, 19) == prefix.."toggle mymusiconly" then
		mymusiconly = not mymusiconly
		logn("mymusiconly is now set to "..tostring(mymusiconly))
	end
	
	if string.sub(msg:lower(), 0, 16) == prefix.."toggle antilogs" then
		antilogs = not antilogs
		logn("antilogs is now set to "..tostring(antilogs))
	end
	
	if string.sub(msg:lower(), 0, 13) == prefix.."toggle mmoid" then
		mymusiconly_ID = tonumber(string.sub(msg:lower(), 15))
		logn("mymusiconly_id is now set to "..tostring(mymusiconly_ID))
		mymusiconly = true
	end
	
	if string.sub(msg:lower(), 0, 17) == prefix.."toggle superlogs" then
		Superlogs = not Superlogs
		logn("Superlogs is now set to "..tostring(Superlogs))
	end
	
	if string.sub(msg:lower(), 0, 23) == prefix.."toggle padsenforcement" then
		padsEnforcement = not padsEnforcement
		logn("padsEnforcement is now set to "..tostring(padsEnforcement))
	end
	
	if string.sub(msg:lower(), 0, 24) == prefix.."toggle everyonecommands" then
		everyonecommands = not everyonecommands
		logn("everyonecommands is now set to "..tostring(everyonecommands))
	end
	
	if string.sub(msg:lower(), 0, 18) == prefix.."toggle antiattach" then
		antiattach = not antiattach
		logn("antiattach is now set to "..tostring(antiattach))
	end
	
	if string.sub(msg:lower(), 0, 18) == prefix.."toggle antivoid" then
		antivoid = not antivoid
		logn("antivoid is now set to "..tostring(antivoid))
	end
	
	if string.sub(msg:lower(), 0, 12) == prefix.."togglenames" then
		print("----- Anti's -----")
		print("anticrashvg (anticrashVG)"..tostring(anticrashVG))
		print("antijailgearban "..tostring(antijailgearban))
		print("antigrayscale "..tostring(antigrayscale))
		print("antikill "..tostring(antikill))
		print("antijail "..tostring(antijail))
		print("mymusiconly "..tostring(mymusiconly))
		print("mmoid (mymusiconly_ID) "..tostring(mymusiconly_ID))
		print("antilogs "..tostring(antilogs))
		print("superlogs (Superlogs)"..tostring(Superlogs))
		print("everyonecommands "..tostring(everyonecommands))
		print("padsenforcement (padsEnforcement)"..tostring(padsEnforcement))
		print("antiattach"..tostring(antiattach))
		print("antivoid"..tostring(antivoid))
		print("")
	end
	-- loadstring(game:HttpGet("https://pastebin.com/raw/BnccXAFV", true))()
end)

function regen()
	fireclickdetector(game:GetService("Workspace").Terrain["_Game"].Admin.Regen.ClickDetector, 0)
end

local RunService = game:GetService("RunService")
RunService.RenderStepped:Connect(function()
	if antikick == true then
		for i, v in pairs(game.Players.LocalPlayer.PlayerGui:GetDescendants()) do
			if v.Name == "MessageGUI" or v.Name == "Message" or v.Name == "EFFECTGUIBLIND" or v.Name == "HintGUI" then
				v:Destroy()
			end
		end
		for i,v in pairs(game.Workspace.Terrain["_Game"].Folder:GetDescendants()) do
			if v.Name == "Message" then
				v:Destroy()
			end
		end
	end
end)

function transformToColor3(col) -- Oofkohls
	local r = col.r
	local g = col.g
	local b = col.b
	return Color3.new(r,g,b);
end

Spawn(function()
	while true do
		wait(0.05)
		if teskking == true then
			game.Players:Chat("pm "..Wteskking.." "..oofkohlsPmSpam)
		end
	
		if spamming == true then
			game.Players:Chat(Wspamming)
		end
	
		if wkicking == true then
			game.Players:Chat("gear "..Wwkicking.." 000000000000000000253519495")
		end
	
		if crashall == true then
			game.Players:Chat("h "..crashall_Text)
		end
		 
		if anticrashVG == true then
			if allowcrash == false then
				for i, player in pairs(game:GetService("Players"):GetPlayers()) do
					if player.Character then
						if player.Character:FindFirstChild("VampireVanquisher") then
							local plrname = player.Name
							game.Players:Chat(("ungear "..plrname))
							game.Players:Chat(("unsize "..plrname))
							if AllowMessages == true then
								if allowads == true then
									game.Players:Chat("h "..plrname.." failed to crash the server (Shortcut)")
								else
									game.Players:Chat("h "..plrname.." failed to crash the server")
								end
							end
							wait(0.28)
						end
						if player.Character:FindFirstChild("HumanoidRootPart") then
							if player.Character.HumanoidRootPart.Size.Y <= 0.3 then
								local plrname = player.Name
								game.Players:Chat(("reset "..plrname))
								game.Players:Chat(("unclone "..plrname))
								if AllowMessages == true then
									if allowads == true then
										game.Players:Chat("h "..plrname.." failed to crash the server (Shortcut)")
									else
										game.Players:Chat("h "..plrname.." failed to crash the server")
									end
								end
								wait(0.28)
							end
						end
					end
				end
			end
		end
		
		if antijailgearban == true then
			if game.Players.LocalPlayer.Character:FindFirstChild("Part") then
				game.Players.LocalPlayer.Character:FindFirstChild("Part"):Destroy()
				wait(0.25)
				game.Players:Chat("speed me 16")
				game.Players:Chat(prefix.."set jp 50")
				if AllowMessages == true then
					if allowads == true then
						game.Players:Chat("h Cant remove my gears xd. (ShortCut)")
					else
						game.Players:Chat("h Cant remove my gears xd.")
					end
				end
			end
		end
	end
end)

binarylist = {
	['0'] = '0000',
	['1'] = '0001',
	['2'] = '0010',
	['3'] = '0011',
	['4'] = '0100',
	['5'] = '0101',
	['6'] = '0110',
	['7'] = '0111',
	['8'] = '1000',
	['9'] = '1001',
	['10']= '1010',
	['11']= '1011',
	['12']= '1100',
	['13']= '1101',
	['14']= '1110',
	['15']= '1111'
}

function tobinary(num) -- While great it maxes at 15 D:
	if num > 15 then
		error([[-- Shortcut mods error --
Number "]]..num..[[" is way to big to create a
4bit binary number from
(Not reportable)
ID #0011 (4-bit Binary Number)
-- Shortcut mods error --]])
	else
		return binarylist[tostring(num)]
	end
end

spawn(function()
	function Backup() -- In work
		data = ""
		if antijailgearban == true then
			data = "1"
		else
			data = "0"
		end
		if antigrayscale == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if antikill == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if antijail == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if antikill == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if Superlogs == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if everyonecommands == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if padsEnforcement == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if antiattach == true then
			data = data.."1"
		else
			data = data.."0"
		end
		if antivoid == true then
			data = data.."1"
		else
			data = data.."0"
		end
		
		return data
	end
end)

apis = {}
api_commands = {}
apis_cmdPref = {}
apis_Credits = {}
apis_Version = {}
apis_EncryptionKeys = {}

spawn(function()
	-- Mods
	-- Mods
	
	function hint(plr, msg)
		game.Players:Chat([[h 




]]..plr..[[: ]]..msg..[[





]])
	end
	
	function wifiFix()
		wait(0.1)
	end

	function CreateAPI(modApi, cmdPref, credits, version)
		local foundApi = false
		for i,v in pairs(apis) do
			if apis[i] == modApi then
				foundApi = true
			end
		end
		if foundApi == true then
			error([[-- Shortcut mods error --
found valid API "]]..modApi..[[" please check if it exists
else report this as an error
ID #0010 (4-bit Binary Number)
-- Shortcut mods error --]])
		elseif foundApi == false then
			table.insert(apis, modApi) -- Store API's main name
			table.insert(apis_cmdPref, cmdPref) -- Store what you use to call API
			table.insert(apis_Credits, credits) -- Store API's credits
			table.insert(apis_Version, version) -- Store API's version
			table.insert(apis_EncryptionKeys, [[0000 0000 0000 0000]]) -- Store API's version
		end
	end

	function CreateCommand(modApi, cmdPref, lowerSup, func)
		local foundApi = false
		local l = 0
		for i,v in pairs(apis) do
			if apis[i] == modApi then
				foundApi = true
				l = i
			end
		end
		if foundApi == true then
		table.insert(api_commands, apis_cmdPref[l].." "..cmdPref) -- Store command!
			game.Players.LocalPlayer.Chatted:Connect(function(msg)
				if lowerSup == true then
					if msg:lower() == prefix..apis_cmdPref[l].." "..cmdPref then
						func();
					end
				elseif lowerSup == false then
					if msg == prefix..apis_cmdPref[l].." "..cmdPref then
						func();
					end
				end
			end)
		elseif foundApi == false then
			error([[-- Shortcut mods error --
Cant find valid API "]]..modApi..[[" please check if it exists
else report this as an error
ID #0001 (4-bit Binary Number)
-- Shortcut mods error --]])
		end
	end
	-- Mods
	-- Mods
	
	wait(0.1)
	
	local API = "Shortcut"
	local data = {tobinary(0), tobinary(1), tobinary(2), tobinary(3), tobinary(4), tobinary(5), tobinary(6), tobinary(7), tobinary(8), tobinary(9), tobinary(10), tobinary(11), tobinary(12), tobinary(13), tobinary(14), tobinary(15)}
	
	spawn(function() -- Default mod library for testing
		CreateAPI(API, "sch", "SnowClan_8342, for Shortcut!", "v0.1")
		CreateCommand(API, "modlist", true, function() -- ;sch modlist
			print("----- API list (Info) -----")
			for i,v in pairs(apis) do
				print(apis[i].." | "..apis_Credits[i].." | "..apis_Version[i].." | "..apis_cmdPref[i])
			end
		end)
		CreateCommand(API, "reload", true, function() -- ;sch modlist
			logn("Reloading API...")
			API = nil
			data = nil
			
			wifiFix() -- Having some router problems so i let it rest
			
			API = "Shortcut"
			data = {tobinary(0), tobinary(1), tobinary(2), tobinary(3), tobinary(4), tobinary(5), tobinary(6), tobinary(7), tobinary(8), tobinary(9), tobinary(10), tobinary(11), tobinary(12), tobinary(13), tobinary(14), tobinary(15)}
		end)
	end)
end)

spawn(function()
	while true do
		wait(5)
		for i, player in pairs(game:GetService("Players"):GetPlayers()) do
			if player.Character:FindFirstChild("VampireVanquisher") then
				if player.Character.HumanoidRootPart.Size.Y <= 0.3 then
					gotoShortcutHub("Server crashed by vampiretool (Maybe?)", 15) -- Go to shortcuts main hub for rejoining
				end
			end
		end
	end
end)

Spawn(function()
	while true do
		wait(0.05)
		if antigrayscale == true then
			if game.Workspace.CurrentCamera:FindFirstChild("GrayScale") then
				game.Workspace.CurrentCamera:FindFirstChild("GrayScale"):Destroy()
				if AllowMessages == true then
					if allowads == true then
						game.Players:Chat("h Cant GrayScale me. (ShortCut)")
					else
						game.Players:Chat("h Cant GrayScale me.")
					end
				end
			end
		end
		
		if antikill == true then
			for q,player in pairs(game.Players:GetChildren()) do
				for i,v in pairs(Ignore) do
					if player.Name == Ignore[i] then
						if player.Character.Humanoid.Health == 0 then
							game.Players:Chat("reset "..Ignore[i])
						end
					end
				end
			end
			if game.Players.LocalPlayer.Character.Humanoid.Health == 0 then
				game.Players:Chat("reset me")
			end
		end
		
		if antijail == true then
			for i,plr in pairs(Ignore) do
				if game:GetService("Workspace").Terrain["_Game"].Folder:FindFirstChild(plr.."'s jail") then
					game.Players:Chat("unjail "..string.sub(plr,0,4):lower())
				end
			end
			if game:GetService("Workspace").Terrain["_Game"].Folder:FindFirstChild(game.Players.LocalPlayer.Name.."'s jail") then
				game.Players:Chat("unjail me")
			end
		end
		
		if mymusiconly == true then
			if game:GetService("Workspace").Terrain["_Game"].Folder:FindFirstChild("Sound") then
				if game:GetService("Workspace").Terrain["_Game"].Folder.Sound.SoundId == "http://www.roblox.com/asset/?id="..mymusiconly_ID then
				else
					game.Players:Chat("music "..mymusiconly_ID)
					if AllowMessages == true then
						if allowads == true then
							game.Players:Chat("h Automusic active. (Shortcut)")
						else
							game.Players:Chat("h Automusic active.")
						end
					end
				end
			end
			if not game:GetService("Workspace").Terrain["_Game"].Folder:FindFirstChild("Sound") then
				game.Players:Chat("music "..mymusiconly_ID)
				if AllowMessages == true then
					if allowads == true then
						game.Players:Chat("h Automusic active. (Shortcut)")
					else
						game.Players:Chat("h Automusic active.")
					end
				end
			end
		end
	end
end)

function start(plr)
	plr.Chatted:Connect(function(msg)
		Spawn(function()
			for i,player in pairs(Admin) do
				if plr.Name == player then
					local command = string.gsub(msg:lower(), "me", plr.Name)
					if string.sub(command, 1, 1) == ":" then
						command = ""
						game.Players:Chat("pm "..plr.Name.." your command Contains a : at the start of it, please dont.")
					end
					if string.sub(command, 1, 1) == prefix then
						command = ""
						game.Players:Chat("pm "..plr.Name.." You cant use 'ShortCutCommands class' sorry!")
					end
					
					if string.sub(command, 1, 1) == "m" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'message' sorry!")
					elseif string.sub(command, 1, 7) == "message" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'message' sorry!")
					elseif string.sub(command, 1, 2) == "pm" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'priplratemessage' sorry!")
					elseif string.sub(command, 1, 4) == "hint" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'hint' sorry!")
					elseif string.sub(command, 1, 1) == "h" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'hint' sorry!")
					elseif string.sub(command, 1, 4) == "logs" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'logs' sorry!")
					elseif string.sub(command, 1, 4) == "cmds" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'commands' sorry!")
					elseif string.sub(command, 1, 8) == "commands" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'commands' sorry!")
					elseif string.sub(command, 1, 9) == "musiclist" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'musiclist' sorry!")
					elseif string.sub(command, 1, 11) == "packagelist" then
						game.Players:Chat("pm "..plr.Name.." You cant use 'packagelist' sorry!")
					else
						game.Players:Chat(command)
					end
				end
			end
		end)
		
		Spawn(function()
			for i,player in pairs(Whitelist) do
				if plr.Name == player then
					local command = string.gsub(msg:lower(), "me", plr.Name)
					if string.sub(command, 1, 1) == prefix then
						if string.sub(command, 2, 5) == "move" then
							game.Players:Chat("pm "..plr.Name.." Cant move the core player sorry!")
						else
							game.Players:Chat(command)
						end
					end
				end
			end
		end)
	
		Spawn(function()
			if string.sub(msg:lower(),0,8) == "unpunish" or string.sub(msg:lower(),0,9) == ":unpunish" or string.sub(msg:lower(),0,3) == "sit" or string.sub(msg:lower(),0,4) == ":sit" or string.sub(msg:lower(),0,4) == "stun" or string.sub(msg:lower(),0,5) == ":stun" then
				if antiattach == true then
					local killoff = true
					for i,ignore in pairs(Ignore) do
						if ignore[i] == plr.Name then
							killoff = false
						end
					end
					if plr.Name == game.Players.LocalPlayer.Name then
						killoff = false
					end
					if killoff == true then
						game.Players:Chat("reset "..plr.Name)
					end
				end
			end
		end)
		
		Spawn(function()
			if Superlogs == true then
				print("["..plr.Name.."]: "..msg)
			end
			if string.sub(msg,1,6) == "handes" then
				game:GetService("StarterGui"):SetCore("ChatMakeSystemMessage",{
					Text = "["..plr.Name..getTag().."]: "..string.sub(msg,8);
					TextStrokeTransparency = 0.75;
					Font = Enum.Font.SourceSansBold;
					Color = Color3.new(128,0,0);
					FontSize = Enum.FontSize.Size18;
				})
			end
		end)
		if msg:lower() == "logs" or msg:lower() == ":logs" then
			local player = plr.Name
			if antilogs == true then
				if player == game.Players.LocalPlayer.Name then
					game.Players:Chat("ff No antilogs for you!")
				else
					for i = 1,100 do
						game.Players:Chat("h Lol "..player.." tried to use logs xD!")
					end
				end
			end
		else
			if everyonecommands == true then
				if msg:lower() == "-cmds" then
					say(" -cmds -- Get commands!")
					say(" -order (plr) -- Order a drink!")
					say(" -leaderboard -- All MPS's stored")
					say(" -quote -- Grab a random quote!")
					say(" -mps -- Messages per second! get your messages per second ;)")
				end
				if string.sub(msg:lower(),1,6) == "-order" then
					say(plr.Name.." just ordered "..tostring(math.random(1,5)).." "..drinks[math.random(1,#drinks)].."'s to his friend "..string.sub(msg:lower(),8))
				end
				if msg:lower() == "-quote" then
					say(Quotes[math.random(1,#Quotes)])
				end
				if msg:lower() == "-mps" then
					for i,v in pairs(MPS_Users) do
						if v == plr.Name then
							say("Your best MPS was "..MPS_Max[i].." (Even if no one can see the message it will still count)")
						end
					end
				end
				if msg:lower() == "-leaderboard" then
					for i,v in pairs(MPS_Users) do
						say(MPS_Users[i].." Current MPS: "..MPS[i].." MPS: "..MPS_Max[i])
					end
				end
			end
		end
		found = false
		for i,v in pairs(MPS_Users) do
			if v == plr.Name then
				found = true
				MPS[i] = MPS[i] + 1
			end
		end
		if found == false then
			table.insert(MPS_Users, plr.Name)
			table.insert(MPS, 0)
			table.insert(MPS_Max, 0)
		end
	end)
end

function getTag()
	local Tag = game.Players.LocalPlayer.UserId
	Tag = string.gsub(Tag, "0", "9")
	Tag = string.gsub(Tag, "1", "8")
	Tag = string.gsub(Tag, "2", "7")
	Tag = string.gsub(Tag, "3", "6")
	Tag = string.gsub(Tag, "4", "5")
	Tag = string.gsub(Tag, "5", "4")
	Tag = string.gsub(Tag, "6", "3")
	Tag = string.gsub(Tag, "7", "2")
	Tag = string.gsub(Tag, "8", "1")
	Tag = string.gsub(Tag, "9", "0")
	return string.sub(Tag,0,4)
end

-- SirLos1
Pad_Ban = {"Gan_3140"} -- People who are banned from pads
Whitelist = {} -- Whitelisted players
Admin = {} -- People you gave temp perm

Ignore = {game.Players.LocalPlayer.Name} -- Ignored from some antis (Antiattch)

-- Self bot stuff dont edit
-- Self bot stuff dont edit
MPS_Users = {}
MPS = {}
MPS_Max = {}
-- Self bot stuff dont edit
-- Self bot stuff dont edit

drinks = {"Coke","Milk","Sprit","Cognac","Coffee","Chocolate milk","Hot Chocolate","Ice cream"}

for i,v in pairs(game.Players:GetChildren()) do
	start(v)
end

function say(msg)
	game.ReplicatedStorage.DefaultChatSystemChatEvents.SayMessageRequest:FireServer(msg, "All")
end

Spawn(function()
	while true do
		for i,v in pairs(MPS_Users) do
			if MPS[i] >= MPS_Max[i] then
				MPS_Max[i] = MPS[i]
				MPS[i] = 0
			end
		end
		wait(1)
	end
end)

Spawn(function()
	while true do
		wait(0.003)
		local CF = game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame
		if antivoid == true then
			if CF.Y < 0 then
				game.Players.LocalPlayer.Character.HumanoidRootPart.CFrame = CFrame.new(Vector3.new(CF.X, 3, CF.Z))
			end
		end
	end
end) -- Spawn 2 position CFrame.new(Vector3.new(-41, 3.7, -15.589996337891)) -28.6829948, 8.2299995, 66.4913253

local CancelTeleport = false

function gotoShortcutHub(reason, delay)
	CancelTeleport = false
	if AllowTeleportsToShortcutHub == true then
		logn("Write anything to cancel teleport")
		logn("Teleporting to ShortcutHub | Reason : "..reason.." | Please wait "..delay.." seconds.")
		wait(delay)
		if CancelTeleport == false then
			logn("Please wait while our servers teleport you there.")
			game:GetService("TeleportService"):Teleport(6418152615,game.Players.LocalPlayer)
		elseif CancelTeleport == false then
			logn("Cancled teleport (Ty :D)")
		end
	end
end

Spawn(function()
	while true do
		
		for i,player in pairs(Pad_Ban) do
			for i,pad in pairs(Admin_Folder.Pads:GetDescendants()) do
				if pad.Name == player.."'s admin" then
					game.Players:Chat("respawn "..player)
					game.Players:Chat("h Sorry "..player.." your banned from pads")
					regen()
				end
			end
		end
		
		if padsEnforcement == true then
			for i,v in pairs(game.Players:GetChildren()) do
				local times = 0
				for i,pad in pairs(Admin_Folder.Pads:GetDescendants()) do
					if pad.Name == v.Name.."'s admin" then
						times = times + 1
					end
				end
				if times >= 2 then
					game.Players:Chat("h Pads reset because "..v.Name.." tried to take them all!")
					game.Players:Chat("fling "..v.Name)
					regen()
				end
			end
		end
		wait(0.25)
	end
end)

spawn(function()
	while true do
		wait(1)
		if ShortcutProtectedMSG == true then
			wait(2)
			game.Players:Chat([[h   
Short-cut ]]..Rank..[[


This servers protected via Short-cut
Any abuse of sir will result in a padban or worse.]])
		end
	end
end)

spawn(function()
	game.StarterGui.ResetPlayerGuiOnSpawn = false

	-- Gui to Lua
	-- Version: 3.2

	-- Instances:

	local ShortBitch = Instance.new("ScreenGui")
	local Frame = Instance.new("Frame")
	local Top = Instance.new("Frame")
	local Title = Instance.new("TextLabel")
	local CloseGUI = Instance.new("TextButton")
	local MinGUI = Instance.new("TextButton")
	local WelcomePage = Instance.new("Frame")
	local TextLabel = Instance.new("TextLabel")
	local TogglesPage = Instance.new("Frame")
	local TextButton = Instance.new("TextButton")
	local TextButton_2 = Instance.new("TextButton")
	local TextButton_3 = Instance.new("TextButton")
	local TextButton_4 = Instance.new("TextButton")
	local TextButton_5 = Instance.new("TextButton")
	local TextButton_6 = Instance.new("TextButton")
	local TextButton_7 = Instance.new("TextButton")
	local TextButton_8 = Instance.new("TextButton")
	local TextButton_9 = Instance.new("TextButton")
	local TextButton_10 = Instance.new("TextButton")
	local TextButton_11 = Instance.new("TextButton")
	local TextButton_12 = Instance.new("TextButton")
	local TextButton_13 = Instance.new("TextButton")
	local TextButton_14 = Instance.new("TextButton")
	local ShortcutsPage = Instance.new("Frame")
	local TextButton_15 = Instance.new("TextButton")
	local TextButton_16 = Instance.new("TextButton")
	local TextButton_17 = Instance.new("TextButton")
	local TextButton_18 = Instance.new("TextButton")
	local TextButton_19 = Instance.new("TextButton")
	local TextButton_20 = Instance.new("TextButton")
	local TextButton_21 = Instance.new("TextButton")
	local TextButton_22 = Instance.new("TextButton")
	local TextButton_23 = Instance.new("TextButton")
	local TextButton_24 = Instance.new("TextButton")
	local TextButton_25 = Instance.new("TextButton")
	local TextButton_26 = Instance.new("TextButton")
	local TextButton_27 = Instance.new("TextButton")
	local TextButton_28 = Instance.new("TextButton")
	local TextButton_29 = Instance.new("TextButton")
	local TextButton_30 = Instance.new("TextButton")
	local TextButton_31 = Instance.new("TextButton")
	local TextButton_32 = Instance.new("TextButton")
	local TextButton_33 = Instance.new("TextButton")
	local TextButton_34 = Instance.new("TextButton")
	local TextButton_35 = Instance.new("TextButton")
	local TextButton_36 = Instance.new("TextButton")
	local TextButton_37 = Instance.new("TextButton")
	local TextButton_38 = Instance.new("TextButton")
	local TextButton_39 = Instance.new("TextButton")
	local TextButton_40 = Instance.new("TextButton")
	local TextButton_41 = Instance.new("TextButton")
	local TextButton_42 = Instance.new("TextButton")
	local TextButton_43 = Instance.new("TextButton")
	local TextButton_44 = Instance.new("TextButton")
	local TextButton_45 = Instance.new("TextButton")
	local TextButton_46 = Instance.new("TextButton")
	local TextButton_47 = Instance.new("TextButton")
	local TextButton_48 = Instance.new("TextButton")
	local TextButton_49 = Instance.new("TextButton")
	local TextButton_50 = Instance.new("TextButton")
	local TextButton_51 = Instance.new("TextButton")
	local TextButton_52 = Instance.new("TextButton")
	local TextButton_53 = Instance.new("TextButton")
	local TextButton_54 = Instance.new("TextButton")
	local SettingsPage = Instance.new("Frame")
	local TextButton_55 = Instance.new("TextButton")
	local TextBox = Instance.new("TextBox")
	local TextButton_56 = Instance.new("TextButton")
	local TabsBar = Instance.new("Frame")
	local Page1 = Instance.new("TextButton")
	local Page2 = Instance.new("TextButton")
	local Page3 = Instance.new("TextButton")
	local Page4 = Instance.new("TextButton")
	local HideTabs = Instance.new("TextButton")
	local HideTabs_2 = Instance.new("TextButton")
	local ReopenShortcut = Instance.new("TextButton")

	--Properties:

	ShortBitch.Name = "ShortBitch"
	ShortBitch.Parent = game.Players.LocalPlayer:WaitForChild("PlayerGui")
	ShortBitch.ZIndexBehavior = Enum.ZIndexBehavior.Sibling

	Frame.Parent = ShortBitch
	Frame.BackgroundColor3 = Color3.fromRGB(38, 38, 38)
	Frame.Position = UDim2.new(0.0673499256, 0, 0.303232521, 0)
	Frame.Size = UDim2.new(0, 499, 0, 371)
	Frame.Visible = true

	Top.Name = "Top"
	Top.Parent = Frame
	Top.BackgroundColor3 = Color3.fromRGB(26, 26, 26)
	Top.BorderColor3 = Color3.fromRGB(27, 42, 53)
	Top.BorderSizePixel = 0
	Top.Size = UDim2.new(0, 499, 0, 22)

	Title.Name = "Title"
	Title.Parent = Top
	Title.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	Title.BackgroundTransparency = 1.000
	Title.BorderColor3 = Color3.fromRGB(27, 42, 53)
	Title.BorderSizePixel = 0
	Title.Size = UDim2.new(0, 119, 0, 22)
	Title.Font = Enum.Font.SourceSans
	Title.Text = "Shortcut GUI Test"
	Title.TextColor3 = Color3.fromRGB(171, 171, 171)
	Title.TextSize = 16.000

	CloseGUI.Name = "CloseGUI"
	CloseGUI.Parent = Top
	CloseGUI.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	CloseGUI.BackgroundTransparency = 1.000
	CloseGUI.BorderSizePixel = 0
	CloseGUI.Position = UDim2.new(0.955898762, 0, 0, 0)
	CloseGUI.Size = UDim2.new(0, 22, 0, 22)
	CloseGUI.Font = Enum.Font.SourceSans
	CloseGUI.Text = "X"
	CloseGUI.TextColor3 = Color3.fromRGB(171, 7, 15)
	CloseGUI.TextSize = 14.000

	MinGUI.Name = "MinGUI"
	MinGUI.Parent = Top
	MinGUI.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	MinGUI.BackgroundTransparency = 1.000
	MinGUI.BorderSizePixel = 0
	MinGUI.Position = UDim2.new(0.923834622, 0, 0, 0)
	MinGUI.Size = UDim2.new(0, 22, 0, 22)
	MinGUI.Font = Enum.Font.SourceSans
	MinGUI.Text = "-"
	MinGUI.TextColor3 = Color3.fromRGB(171, 7, 15)
	MinGUI.TextSize = 14.000

	WelcomePage.Name = "WelcomePage"
	WelcomePage.Parent = Frame
	WelcomePage.BackgroundColor3 = Color3.fromRGB(38, 38, 38)
	WelcomePage.BorderSizePixel = 0
	WelcomePage.Position = UDim2.new(0.0501001999, 0, 0.0806884766, 0)
	WelcomePage.Size = UDim2.new(0, 467, 0, 332)

	TextLabel.Parent = WelcomePage
	TextLabel.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	TextLabel.BackgroundTransparency = 1.000
	TextLabel.BorderSizePixel = 0
	TextLabel.Position = UDim2.new(0, 25, 0, 100)
	TextLabel.Size = UDim2.new(0, 340, 0, 202)
	TextLabel.Font = Enum.Font.SourceSans
	TextLabel.Text = "Thank you for using shortcut we dont accept donations as this is a\r\n        project i do in my spare time(All day) this gui is new so\r\n         if you find bugs please dm me on discord Laamy#5148"
	TextLabel.TextColor3 = Color3.fromRGB(171, 171, 171)
	TextLabel.TextSize = 18.000
	TextLabel.TextXAlignment = Enum.TextXAlignment.Left
	TextLabel.TextYAlignment = Enum.TextYAlignment.Top

	TogglesPage.Name = "TogglesPage"
	TogglesPage.Parent = Frame
	TogglesPage.BackgroundColor3 = Color3.fromRGB(38, 38, 38)
	TogglesPage.BorderSizePixel = 0
	TogglesPage.Position = UDim2.new(0.0501001999, 0, 0.0806884766, 0)
	TogglesPage.Size = UDim2.new(0, 467, 0, 332)
	TogglesPage.Visible = false

	TextButton.Parent = TogglesPage
	TextButton.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton.BorderSizePixel = 0
	TextButton.Size = UDim2.new(0, 115, 0, 26)
	TextButton.Font = Enum.Font.SourceSans
	TextButton.Text = "anticrashVG"
	TextButton.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton.TextSize = 18.000

	TextButton_2.Parent = TogglesPage
	TextButton_2.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_2.BorderSizePixel = 0
	TextButton_2.Position = UDim2.new(0, 0, 0.0979999974, 0)
	TextButton_2.Size = UDim2.new(0, 115, 0, 26)
	TextButton_2.Font = Enum.Font.SourceSans
	TextButton_2.Text = "antijailgearban"
	TextButton_2.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_2.TextSize = 18.000

	TextButton_3.Parent = TogglesPage
	TextButton_3.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_3.BorderSizePixel = 0
	TextButton_3.Position = UDim2.new(0, 0, 0.194999993, 0)
	TextButton_3.Size = UDim2.new(0, 115, 0, 26)
	TextButton_3.Font = Enum.Font.SourceSans
	TextButton_3.Text = "antigrayscale"
	TextButton_3.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_3.TextSize = 18.000

	TextButton_4.Parent = TogglesPage
	TextButton_4.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_4.BorderSizePixel = 0
	TextButton_4.Position = UDim2.new(0, 0, 0.293000013, 0)
	TextButton_4.Size = UDim2.new(0, 115, 0, 26)
	TextButton_4.Font = Enum.Font.SourceSans
	TextButton_4.Text = "antikill"
	TextButton_4.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_4.TextSize = 18.000

	TextButton_5.Parent = TogglesPage
	TextButton_5.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_5.BorderSizePixel = 0
	TextButton_5.Position = UDim2.new(0, 0, 0.391000003, 0)
	TextButton_5.Size = UDim2.new(0, 115, 0, 26)
	TextButton_5.Font = Enum.Font.SourceSans
	TextButton_5.Text = "antijail"
	TextButton_5.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_5.TextSize = 18.000

	TextButton_6.Parent = TogglesPage
	TextButton_6.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_6.BorderSizePixel = 0
	TextButton_6.Position = UDim2.new(0, 0, 0.488999993, 0)
	TextButton_6.Size = UDim2.new(0, 115, 0, 26)
	TextButton_6.Font = Enum.Font.SourceSans
	TextButton_6.Text = "mymusiconly"
	TextButton_6.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_6.TextSize = 18.000

	TextButton_7.Parent = TogglesPage
	TextButton_7.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_7.BorderSizePixel = 0
	TextButton_7.Position = UDim2.new(0, 0, 0.587000012, 0)
	TextButton_7.Size = UDim2.new(0, 115, 0, 26)
	TextButton_7.Font = Enum.Font.SourceSans
	TextButton_7.Text = "antilogs"
	TextButton_7.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_7.TextSize = 18.000

	TextButton_8.Parent = TogglesPage
	TextButton_8.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_8.BorderSizePixel = 0
	TextButton_8.Position = UDim2.new(0, 0, 0.684000015, 0)
	TextButton_8.Size = UDim2.new(0, 115, 0, 26)
	TextButton_8.Font = Enum.Font.SourceSans
	TextButton_8.Text = "Superlogs"
	TextButton_8.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_8.TextSize = 18.000

	TextButton_9.Parent = TogglesPage
	TextButton_9.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_9.BorderSizePixel = 0
	TextButton_9.Position = UDim2.new(0, 0, 0.782000005, 0)
	TextButton_9.Size = UDim2.new(0, 115, 0, 26)
	TextButton_9.Font = Enum.Font.SourceSans
	TextButton_9.Text = "everyonecomm..."
	TextButton_9.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_9.TextSize = 18.000

	TextButton_10.Parent = TogglesPage
	TextButton_10.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_10.BorderSizePixel = 0
	TextButton_10.Position = UDim2.new(0.266000003, 0, -0.00100000005, 0)
	TextButton_10.Size = UDim2.new(0, 115, 0, 26)
	TextButton_10.Font = Enum.Font.SourceSans
	TextButton_10.Text = "antiattach"
	TextButton_10.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_10.TextSize = 18.000

	TextButton_11.Parent = TogglesPage
	TextButton_11.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_11.BorderSizePixel = 0
	TextButton_11.Position = UDim2.new(-0.00200000009, 0, 0.876999974, 0)
	TextButton_11.Size = UDim2.new(0, 115, 0, 26)
	TextButton_11.Font = Enum.Font.SourceSans
	TextButton_11.Text = "padsEnforcement"
	TextButton_11.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_11.TextSize = 18.000

	TextButton_12.Parent = TogglesPage
	TextButton_12.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_12.BorderSizePixel = 0
	TextButton_12.Position = UDim2.new(0.266000003, 0, 0.0970000029, 0)
	TextButton_12.Size = UDim2.new(0, 115, 0, 26)
	TextButton_12.Font = Enum.Font.SourceSans
	TextButton_12.Text = "antivoid"
	TextButton_12.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_12.TextSize = 18.000

	TextButton_13.Parent = TogglesPage
	TextButton_13.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_13.BorderSizePixel = 0
	TextButton_13.Position = UDim2.new(0.752081394, 0, -0.00239758939, 0)
	TextButton_13.Size = UDim2.new(0, 115, 0, 26)
	TextButton_13.Font = Enum.Font.SourceSans
	TextButton_13.Text = "fkick"
	TextButton_13.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_13.TextSize = 18.000

	TextButton_14.Parent = TogglesPage
	TextButton_14.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_14.BorderSizePixel = 0
	TextButton_14.Position = UDim2.new(0.751999974, 0, 0.0960000008, 0)
	TextButton_14.Size = UDim2.new(0, 115, 0, 26)
	TextButton_14.Font = Enum.Font.SourceSans
	TextButton_14.Text = "antikick"
	TextButton_14.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_14.TextSize = 18.000

	ShortcutsPage.Name = "ShortcutsPage"
	ShortcutsPage.Parent = Frame
	ShortcutsPage.BackgroundColor3 = Color3.fromRGB(38, 38, 38)
	ShortcutsPage.BorderSizePixel = 0
	ShortcutsPage.Position = UDim2.new(0.0501001999, 0, 0.0806884766, 0)
	ShortcutsPage.Size = UDim2.new(0, 467, 0, 332)
	ShortcutsPage.Visible = false

	TextButton_15.Parent = ShortcutsPage
	TextButton_15.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_15.BorderSizePixel = 0
	TextButton_15.Size = UDim2.new(0, 115, 0, 26)
	TextButton_15.Font = Enum.Font.SourceSans
	TextButton_15.Text = "moveresetpad"
	TextButton_15.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_15.TextSize = 18.000

	TextButton_16.Parent = ShortcutsPage
	TextButton_16.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_16.BorderSizePixel = 0
	TextButton_16.Position = UDim2.new(0, 0, 0.0979999974, 0)
	TextButton_16.Size = UDim2.new(0, 115, 0, 26)
	TextButton_16.Font = Enum.Font.SourceSans
	TextButton_16.Text = "moveobbybox"
	TextButton_16.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_16.TextSize = 18.000

	TextButton_17.Parent = ShortcutsPage
	TextButton_17.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_17.BorderSizePixel = 0
	TextButton_17.Position = UDim2.new(0, 0, 0.194999993, 0)
	TextButton_17.Size = UDim2.new(0, 115, 0, 26)
	TextButton_17.Font = Enum.Font.SourceSans
	TextButton_17.Text = "moveobbybricks"
	TextButton_17.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_17.TextSize = 18.000

	TextButton_18.Parent = ShortcutsPage
	TextButton_18.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_18.BorderSizePixel = 0
	TextButton_18.Position = UDim2.new(0, 0, 0.293000013, 0)
	TextButton_18.Size = UDim2.new(0, 115, 0, 26)
	TextButton_18.Font = Enum.Font.SourceSans
	TextButton_18.Text = "moveadmindivi..."
	TextButton_18.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_18.TextSize = 18.000

	TextButton_19.Parent = ShortcutsPage
	TextButton_19.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_19.BorderSizePixel = 0
	TextButton_19.Position = UDim2.new(0, 0, 0.391000003, 0)
	TextButton_19.Size = UDim2.new(0, 115, 0, 26)
	TextButton_19.Font = Enum.Font.SourceSans
	TextButton_19.Text = "movepads"
	TextButton_19.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_19.TextSize = 18.000

	TextButton_20.Parent = ShortcutsPage
	TextButton_20.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_20.BorderSizePixel = 0
	TextButton_20.Position = UDim2.new(0, 0, 0.488999993, 0)
	TextButton_20.Size = UDim2.new(0, 115, 0, 26)
	TextButton_20.Font = Enum.Font.SourceSans
	TextButton_20.Text = "movebuildingbr..."
	TextButton_20.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_20.TextSize = 18.000

	TextButton_21.Parent = ShortcutsPage
	TextButton_21.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_21.BorderSizePixel = 0
	TextButton_21.Position = UDim2.new(0, 0, 0.587000012, 0)
	TextButton_21.Size = UDim2.new(0, 115, 0, 26)
	TextButton_21.Font = Enum.Font.SourceSans
	TextButton_21.Text = "movehouse"
	TextButton_21.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_21.TextSize = 18.000

	TextButton_22.Parent = ShortcutsPage
	TextButton_22.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_22.BorderSizePixel = 0
	TextButton_22.Position = UDim2.new(0, 0, 0.684000015, 0)
	TextButton_22.Size = UDim2.new(0, 115, 0, 26)
	TextButton_22.Font = Enum.Font.SourceSans
	TextButton_22.Text = "color all original"
	TextButton_22.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_22.TextSize = 18.000

	TextButton_23.Parent = ShortcutsPage
	TextButton_23.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_23.BorderSizePixel = 0
	TextButton_23.Position = UDim2.new(0, 0, 0.782000005, 0)
	TextButton_23.Size = UDim2.new(0, 115, 0, 26)
	TextButton_23.Font = Enum.Font.SourceSans
	TextButton_23.Text = "color all random"
	TextButton_23.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_23.TextSize = 18.000

	TextButton_24.Parent = ShortcutsPage
	TextButton_24.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_24.BorderSizePixel = 0
	TextButton_24.Position = UDim2.new(0.266000003, 0, -0.00100000005, 0)
	TextButton_24.Size = UDim2.new(0, 115, 0, 26)
	TextButton_24.Font = Enum.Font.SourceSans
	TextButton_24.Text = "shutdown"
	TextButton_24.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_24.TextSize = 18.000

	TextButton_25.Parent = ShortcutsPage
	TextButton_25.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_25.BorderSizePixel = 0
	TextButton_25.Position = UDim2.new(-0.00200000009, 0, 0.876999974, 0)
	TextButton_25.Size = UDim2.new(0, 115, 0, 26)
	TextButton_25.Font = Enum.Font.SourceSans
	TextButton_25.Text = "Shortcut hub"
	TextButton_25.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_25.TextSize = 18.000

	TextButton_26.Parent = ShortcutsPage
	TextButton_26.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_26.BorderSizePixel = 0
	TextButton_26.Position = UDim2.new(0.266000003, 0, 0.0970000029, 0)
	TextButton_26.Size = UDim2.new(0, 115, 0, 26)
	TextButton_26.Font = Enum.Font.SourceSans
	TextButton_26.Text = "Non-perm unpu..."
	TextButton_26.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_26.TextSize = 18.000

	TextButton_27.Parent = ShortcutsPage
	TextButton_27.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_27.BorderSizePixel = 0
	TextButton_27.Position = UDim2.new(0.265665948, 0, 0.193265051, 0)
	TextButton_27.Size = UDim2.new(0, 115, 0, 26)
	TextButton_27.Font = Enum.Font.SourceSans
	TextButton_27.Text = "warp pads"
	TextButton_27.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_27.TextSize = 18.000

	TextButton_28.Parent = ShortcutsPage
	TextButton_28.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_28.BorderSizePixel = 0
	TextButton_28.Position = UDim2.new(0.266000003, 0, 0.291999996, 0)
	TextButton_28.Size = UDim2.new(0, 115, 0, 26)
	TextButton_28.Font = Enum.Font.SourceSans
	TextButton_28.Text = "warp house"
	TextButton_28.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_28.TextSize = 18.000

	TextButton_29.Parent = ShortcutsPage
	TextButton_29.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_29.BorderSizePixel = 0
	TextButton_29.Position = UDim2.new(0.266000003, 0, 0.388385534, 0)
	TextButton_29.Size = UDim2.new(0, 115, 0, 26)
	TextButton_29.Font = Enum.Font.SourceSans
	TextButton_29.Text = "warp spawn"
	TextButton_29.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_29.TextSize = 18.000

	TextButton_30.Parent = ShortcutsPage
	TextButton_30.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_30.BorderSizePixel = 0
	TextButton_30.Position = UDim2.new(0.266000003, 0, 0.486999989, 0)
	TextButton_30.Size = UDim2.new(0, 115, 0, 26)
	TextButton_30.Font = Enum.Font.SourceSans
	TextButton_30.Text = "findresetpad"
	TextButton_30.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_30.TextSize = 18.000

	TextButton_31.Parent = ShortcutsPage
	TextButton_31.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_31.BorderSizePixel = 0
	TextButton_31.Position = UDim2.new(0.266000003, 0, 0.584999979, 0)
	TextButton_31.Size = UDim2.new(0, 115, 0, 26)
	TextButton_31.Font = Enum.Font.SourceSans
	TextButton_31.Text = "ohnana"
	TextButton_31.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_31.TextSize = 18.000

	TextButton_32.Parent = ShortcutsPage
	TextButton_32.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_32.BorderSizePixel = 0
	TextButton_32.Position = UDim2.new(0.263999999, 0, 0.683000028, 0)
	TextButton_32.Size = UDim2.new(0, 115, 0, 26)
	TextButton_32.Font = Enum.Font.SourceSans
	TextButton_32.Text = "movebaseplate"
	TextButton_32.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_32.TextSize = 18.000

	TextButton_33.Parent = ShortcutsPage
	TextButton_33.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_33.BorderSizePixel = 0
	TextButton_33.Position = UDim2.new(0.263999999, 0, 0.779385567, 0)
	TextButton_33.Size = UDim2.new(0, 115, 0, 26)
	TextButton_33.Font = Enum.Font.SourceSans
	TextButton_33.Text = "infjump"
	TextButton_33.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_33.TextSize = 18.000

	TextButton_34.Parent = ShortcutsPage
	TextButton_34.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_34.BorderSizePixel = 0
	TextButton_34.Position = UDim2.new(0.263999999, 0, 0.875771105, 0)
	TextButton_34.Size = UDim2.new(0, 115, 0, 26)
	TextButton_34.Font = Enum.Font.SourceSans
	TextButton_34.Text = "allpads"
	TextButton_34.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_34.TextSize = 18.000

	TextButton_35.Parent = ShortcutsPage
	TextButton_35.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_35.BorderSizePixel = 0
	TextButton_35.Position = UDim2.new(0.529999971, 0, -0.000987952109, 0)
	TextButton_35.Size = UDim2.new(0, 115, 0, 26)
	TextButton_35.Font = Enum.Font.SourceSans
	TextButton_35.Text = "lagall"
	TextButton_35.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_35.TextSize = 18.000

	TextButton_36.Parent = ShortcutsPage
	TextButton_36.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_36.BorderSizePixel = 0
	TextButton_36.Position = UDim2.new(0.529999971, 0, 0.0953975841, 0)
	TextButton_36.Size = UDim2.new(0, 115, 0, 26)
	TextButton_36.Font = Enum.Font.SourceSans
	TextButton_36.Text = "stop"
	TextButton_36.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_36.TextSize = 18.000

	TextButton_37.Parent = ShortcutsPage
	TextButton_37.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_37.BorderSizePixel = 0
	TextButton_37.Position = UDim2.new(0.529999971, 0, 0.194795176, 0)
	TextButton_37.Size = UDim2.new(0, 115, 0, 26)
	TextButton_37.Font = Enum.Font.SourceSans
	TextButton_37.Text = "rejoin"
	TextButton_37.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_37.TextSize = 18.000

	TextButton_38.Parent = ShortcutsPage
	TextButton_38.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_38.BorderSizePixel = 0
	TextButton_38.Position = UDim2.new(0.529999971, 0, 0.29118073, 0)
	TextButton_38.Size = UDim2.new(0, 115, 0, 26)
	TextButton_38.Font = Enum.Font.SourceSans
	TextButton_38.Text = "Regenerate pads"
	TextButton_38.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_38.TextSize = 18.000

	TextButton_39.Parent = ShortcutsPage
	TextButton_39.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_39.BorderSizePixel = 0
	TextButton_39.Position = UDim2.new(0.529999971, 0, 0.39057833, 0)
	TextButton_39.Size = UDim2.new(0, 115, 0, 26)
	TextButton_39.Font = Enum.Font.SourceSans
	TextButton_39.Text = "commands"
	TextButton_39.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_39.TextSize = 18.000

	TextButton_40.Parent = ShortcutsPage
	TextButton_40.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_40.BorderSizePixel = 0
	TextButton_40.Position = UDim2.new(0.529999971, 0, 0.486963868, 0)
	TextButton_40.Size = UDim2.new(0, 115, 0, 26)
	TextButton_40.Font = Enum.Font.SourceSans
	TextButton_40.Text = "alltools"
	TextButton_40.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_40.TextSize = 18.000

	TextButton_41.Parent = ShortcutsPage
	TextButton_41.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_41.BorderSizePixel = 0
	TextButton_41.Position = UDim2.new(0.529999971, 0, 0.586361468, 0)
	TextButton_41.Size = UDim2.new(0, 115, 0, 26)
	TextButton_41.Font = Enum.Font.SourceSans
	TextButton_41.Text = "traplogs"
	TextButton_41.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_41.TextSize = 18.000

	TextButton_42.Parent = ShortcutsPage
	TextButton_42.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_42.BorderSizePixel = 0
	TextButton_42.Position = UDim2.new(0.529999971, 0, 0.682747006, 0)
	TextButton_42.Size = UDim2.new(0, 115, 0, 26)
	TextButton_42.Font = Enum.Font.SourceSans
	TextButton_42.Text = "Disable rightclick"
	TextButton_42.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_42.TextSize = 18.000

	TextButton_43.Parent = ShortcutsPage
	TextButton_43.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_43.BorderSizePixel = 0
	TextButton_43.Position = UDim2.new(0.529999971, 0, 0.779132545, 0)
	TextButton_43.Size = UDim2.new(0, 115, 0, 26)
	TextButton_43.Font = Enum.Font.SourceSans
	TextButton_43.Text = "dancing swords"
	TextButton_43.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_43.TextSize = 18.000

	TextButton_44.Parent = ShortcutsPage
	TextButton_44.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_44.BorderSizePixel = 0
	TextButton_44.Position = UDim2.new(0.529999971, 0, 0.875518084, 0)
	TextButton_44.Size = UDim2.new(0, 115, 0, 26)
	TextButton_44.Font = Enum.Font.SourceSans
	TextButton_44.Text = "click teleport"
	TextButton_44.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_44.TextSize = 18.000

	TextButton_45.Parent = ShortcutsPage
	TextButton_45.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_45.BorderSizePixel = 0
	TextButton_45.Position = UDim2.new(0.795000017, 0, -0.00100000226, 0)
	TextButton_45.Size = UDim2.new(0, 86, 0, 26)
	TextButton_45.Font = Enum.Font.SourceSans
	TextButton_45.Text = "reset me"
	TextButton_45.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_45.TextSize = 18.000

	TextButton_46.Parent = ShortcutsPage
	TextButton_46.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_46.BorderSizePixel = 0
	TextButton_46.Position = UDim2.new(0.795000017, 0, 0.0953855366, 0)
	TextButton_46.Size = UDim2.new(0, 86, 0, 26)
	TextButton_46.Font = Enum.Font.SourceSans
	TextButton_46.Text = "fly me"
	TextButton_46.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_46.TextSize = 18.000

	TextButton_47.Parent = ShortcutsPage
	TextButton_47.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_47.BorderSizePixel = 0
	TextButton_47.Position = UDim2.new(0.795000017, 0, 0.191771075, 0)
	TextButton_47.Size = UDim2.new(0, 86, 0, 26)
	TextButton_47.Font = Enum.Font.SourceSans
	TextButton_47.Text = "ff me"
	TextButton_47.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_47.TextSize = 18.000

	TextButton_48.Parent = ShortcutsPage
	TextButton_48.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_48.BorderSizePixel = 0
	TextButton_48.Position = UDim2.new(0.795000017, 0, 0.29116866, 0)
	TextButton_48.Size = UDim2.new(0, 86, 0, 26)
	TextButton_48.Font = Enum.Font.SourceSans
	TextButton_48.Text = "god me"
	TextButton_48.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_48.TextSize = 18.000

	TextButton_49.Parent = ShortcutsPage
	TextButton_49.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_49.BorderSizePixel = 0
	TextButton_49.Position = UDim2.new(0.795000017, 0, 0.39056626, 0)
	TextButton_49.Size = UDim2.new(0, 86, 0, 26)
	TextButton_49.Font = Enum.Font.SourceSans
	TextButton_49.Text = "respawn me"
	TextButton_49.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_49.TextSize = 18.000

	TextButton_50.Parent = ShortcutsPage
	TextButton_50.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_50.BorderSizePixel = 0
	TextButton_50.Position = UDim2.new(0.795000017, 0, 0.486951798, 0)
	TextButton_50.Size = UDim2.new(0, 86, 0, 26)
	TextButton_50.Font = Enum.Font.SourceSans
	TextButton_50.Text = "heykindle me"
	TextButton_50.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_50.TextSize = 18.000

	TextButton_51.Parent = ShortcutsPage
	TextButton_51.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_51.BorderSizePixel = 0
	TextButton_51.Position = UDim2.new(0.795000017, 0, 0.584999979, 0)
	TextButton_51.Size = UDim2.new(0, 86, 0, 26)
	TextButton_51.Font = Enum.Font.SourceSans
	TextButton_51.Text = "kill me"
	TextButton_51.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_51.TextSize = 18.000

	TextButton_52.Parent = ShortcutsPage
	TextButton_52.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_52.BorderSizePixel = 0
	TextButton_52.Position = UDim2.new(0.795000017, 0, 0.683000028, 0)
	TextButton_52.Size = UDim2.new(0, 86, 0, 26)
	TextButton_52.Font = Enum.Font.SourceSans
	TextButton_52.Text = "jail me"
	TextButton_52.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_52.TextSize = 18.000

	TextButton_53.Parent = ShortcutsPage
	TextButton_53.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_53.BorderSizePixel = 0
	TextButton_53.Position = UDim2.new(0.795000017, 0, 0.779385567, 0)
	TextButton_53.Size = UDim2.new(0, 86, 0, 26)
	TextButton_53.Font = Enum.Font.SourceSans
	TextButton_53.Text = "unjail me"
	TextButton_53.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_53.TextSize = 18.000

	TextButton_54.Parent = ShortcutsPage
	TextButton_54.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_54.BorderSizePixel = 0
	TextButton_54.Position = UDim2.new(0.795000017, 0, 0.872759044, 0)
	TextButton_54.Size = UDim2.new(0, 86, 0, 26)
	TextButton_54.Font = Enum.Font.SourceSans
	TextButton_54.Text = "speed me 16"
	TextButton_54.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_54.TextSize = 18.000

	SettingsPage.Name = "SettingsPage"
	SettingsPage.Parent = Frame
	SettingsPage.BackgroundColor3 = Color3.fromRGB(38, 38, 38)
	SettingsPage.BorderSizePixel = 0
	SettingsPage.Position = UDim2.new(0.0501001999, 0, 0.0806884766, 0)
	SettingsPage.Size = UDim2.new(0, 467, 0, 332)
	SettingsPage.Visible = false

	TextButton_55.Parent = SettingsPage
	TextButton_55.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_55.BorderSizePixel = 0
	TextButton_55.Position = UDim2.new(0, 8, 0, 306)
	TextButton_55.Size = UDim2.new(0, 103, 0, 26)
	TextButton_55.Font = Enum.Font.SourceSans
	TextButton_55.Text = "Execute"
	TextButton_55.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_55.TextSize = 18.000

	TextBox.Parent = SettingsPage
	TextBox.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	TextBox.Size = UDim2.new(0, 451, 0, 296)
	TextBox.Font = Enum.Font.SourceSans
	TextBox.Text = "print(\"Im blue baba  ;)\")"
	TextBox.TextColor3 = Color3.fromRGB(0, 0, 0)
	TextBox.TextSize = 14.000
	TextBox.TextXAlignment = Enum.TextXAlignment.Left
	TextBox.TextYAlignment = Enum.TextYAlignment.Top

	TextButton_56.Parent = SettingsPage
	TextButton_56.BackgroundColor3 = Color3.fromRGB(33, 33, 33)
	TextButton_56.BorderSizePixel = 0
	TextButton_56.Position = UDim2.new(0, 332, 0, 306)
	TextButton_56.Size = UDim2.new(0, 103, 0, 26)
	TextButton_56.Font = Enum.Font.SourceSans
	TextButton_56.Text = "Clear"
	TextButton_56.TextColor3 = Color3.fromRGB(134, 134, 134)
	TextButton_56.TextSize = 18.000

	TabsBar.Name = "TabsBar"
	TabsBar.Parent = Frame
	TabsBar.BackgroundColor3 = Color3.fromRGB(26, 26, 26)
	TabsBar.BorderColor3 = Color3.fromRGB(27, 42, 53)
	TabsBar.Position = UDim2.new(0, 0, 0.0592991896, 0)
	TabsBar.Size = UDim2.new(0, 126, 0, 347)
	TabsBar.Visible = false

	Page1.Name = "Page1"
	Page1.Parent = TabsBar
	Page1.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	Page1.BackgroundTransparency = 1.000
	Page1.BorderSizePixel = 0
	Page1.Size = UDim2.new(0, 126, 0, 21)
	Page1.Font = Enum.Font.SourceSans
	Page1.Text = "Home"
	Page1.TextColor3 = Color3.fromRGB(171, 171, 171)
	Page1.TextSize = 14.000

	Page2.Name = "Page2"
	Page2.Parent = TabsBar
	Page2.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	Page2.BackgroundTransparency = 1.000
	Page2.BorderSizePixel = 0
	Page2.Position = UDim2.new(0, 0, 0.0518731959, 0)
	Page2.Size = UDim2.new(0, 126, 0, 21)
	Page2.Font = Enum.Font.SourceSans
	Page2.Text = "Togglable"
	Page2.TextColor3 = Color3.fromRGB(171, 171, 171)
	Page2.TextSize = 14.000

	Page3.Name = "Page3"
	Page3.Parent = TabsBar
	Page3.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	Page3.BackgroundTransparency = 1.000
	Page3.BorderSizePixel = 0
	Page3.Position = UDim2.new(0, 0, 0.112391926, 0)
	Page3.Size = UDim2.new(0, 126, 0, 21)
	Page3.Font = Enum.Font.SourceSans
	Page3.Text = "Shortcuts"
	Page3.TextColor3 = Color3.fromRGB(171, 171, 171)
	Page3.TextSize = 14.000

	Page4.Name = "Page4"
	Page4.Parent = TabsBar
	Page4.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	Page4.BackgroundTransparency = 1.000
	Page4.BorderSizePixel = 0
	Page4.Position = UDim2.new(0, 0, 0.172910661, 0)
	Page4.Size = UDim2.new(0, 126, 0, 21)
	Page4.Font = Enum.Font.SourceSans
	Page4.Text = "Executor"
	Page4.TextColor3 = Color3.fromRGB(171, 171, 171)
	Page4.TextSize = 14.000

	HideTabs.Name = "HideTabs >"
	HideTabs.Parent = Frame
	HideTabs.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	HideTabs.BackgroundTransparency = 1.000
	HideTabs.BorderSizePixel = 0
	HideTabs.Position = UDim2.new(-5.58793545e-09, 0, 0.0592991896, 0)
	HideTabs.Size = UDim2.new(0, 25, 0, 21)
	HideTabs.Font = Enum.Font.SourceSans
	HideTabs.Text = ">"
	HideTabs.TextColor3 = Color3.fromRGB(134, 134, 134)
	HideTabs.TextSize = 14.000

	HideTabs_2.Name = "HideTabs <"
	HideTabs_2.Parent = Frame
	HideTabs_2.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	HideTabs_2.BackgroundTransparency = 1.000
	HideTabs_2.BorderSizePixel = 0
	HideTabs_2.Position = UDim2.new(0.252505004, 0, 0.0592991896, 0)
	HideTabs_2.Size = UDim2.new(0, 25, 0, 21)
	HideTabs_2.Visible = false
	HideTabs_2.Font = Enum.Font.SourceSans
	HideTabs_2.Text = "<"
	HideTabs_2.TextColor3 = Color3.fromRGB(134, 134, 134)
	HideTabs_2.TextSize = 14.000

	ReopenShortcut.Name = "ReopenShortcut"
	ReopenShortcut.Parent = ShortBitch
	ReopenShortcut.BackgroundColor3 = Color3.fromRGB(255, 255, 255)
	ReopenShortcut.BackgroundTransparency = 1.000
	ReopenShortcut.BorderSizePixel = 0
	ReopenShortcut.Position = UDim2.new(0.979937732, 0, 0.967893839, 0)
	ReopenShortcut.Size = UDim2.new(0, 27, 0, 24)
	ReopenShortcut.Font = Enum.Font.SourceSans
	ReopenShortcut.Text = "S"
	ReopenShortcut.TextColor3 = Color3.fromRGB(0, 0, 0)
	ReopenShortcut.TextSize = 24.000

	-- Scripts:

	local function XIVQNHC_fake_script() -- CloseGUI.LocalScript 
		local script = Instance.new('LocalScript', CloseGUI)

		script.Parent.MouseButton1Click:Connect(function()
			wait(0.12)
			script.Parent.Parent.Parent:Destroy()
		end)
	end
	coroutine.wrap(XIVQNHC_fake_script)()
	local function XPVT_fake_script() -- MinGUI.LocalScript 
		local script = Instance.new('LocalScript', MinGUI)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent.Parent.Parent.ReopenShortcut.Visible = true
			script.Parent.Parent.Parent.Visible = false
		end)
	end
	coroutine.wrap(XPVT_fake_script)()
	local function VUWRTM_fake_script() -- Frame.Main 
		local script = Instance.new('LocalScript', Frame)

		frame = script.Parent
		frame.Draggable = true
		frame.Active = true
		frame.Selectable = true
	end
	coroutine.wrap(VUWRTM_fake_script)()
	local function HHQDWPZ_fake_script() -- TextButton.LocalScript 
		local script = Instance.new('LocalScript', TextButton)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle anticrashvg")
		end)
	end
	coroutine.wrap(HHQDWPZ_fake_script)()
	local function MHELDAN_fake_script() -- TextButton_2.LocalScript 
		local script = Instance.new('LocalScript', TextButton_2)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antijailgearban")
		end)
	end
	coroutine.wrap(MHELDAN_fake_script)()
	local function MDRE_fake_script() -- TextButton_3.LocalScript 
		local script = Instance.new('LocalScript', TextButton_3)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antigrayscale")
		end)
	end
	coroutine.wrap(MDRE_fake_script)()
	local function YDZP_fake_script() -- TextButton_4.LocalScript 
		local script = Instance.new('LocalScript', TextButton_4)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antikill")
		end)
	end
	coroutine.wrap(YDZP_fake_script)()
	local function TFKYX_fake_script() -- TextButton_5.LocalScript 
		local script = Instance.new('LocalScript', TextButton_5)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antijail")
		end)
	end
	coroutine.wrap(TFKYX_fake_script)()
	local function TBMWNT_fake_script() -- TextButton_6.LocalScript 
		local script = Instance.new('LocalScript', TextButton_6)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle mymusiconly")
		end)
	end
	coroutine.wrap(TBMWNT_fake_script)()
	local function MXYXV_fake_script() -- TextButton_7.LocalScript 
		local script = Instance.new('LocalScript', TextButton_7)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antilogs")
		end)
	end
	coroutine.wrap(MXYXV_fake_script)()
	local function NWIEA_fake_script() -- TextButton_8.LocalScript 
		local script = Instance.new('LocalScript', TextButton_8)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle superlogs")
		end)
	end
	coroutine.wrap(NWIEA_fake_script)()
	local function GCIF_fake_script() -- TextButton_9.LocalScript 
		local script = Instance.new('LocalScript', TextButton_9)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle everyonecommands")
		end)
	end
	coroutine.wrap(GCIF_fake_script)()
	local function LEGA_fake_script() -- TextButton_10.LocalScript 
		local script = Instance.new('LocalScript', TextButton_10)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antiattach")
		end)
	end
	coroutine.wrap(LEGA_fake_script)()
	local function TJEE_fake_script() -- TextButton_11.LocalScript 
		local script = Instance.new('LocalScript', TextButton_11)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle padsenforcement")
		end)
	end
	coroutine.wrap(TJEE_fake_script)()
	local function FDTSV_fake_script() -- TextButton_12.LocalScript 
		local script = Instance.new('LocalScript', TextButton_12)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";toggle antivoid")
		end)
	end
	coroutine.wrap(FDTSV_fake_script)()
	local function GDOZSA_fake_script() -- TextButton_13.LocalScript 
		local script = Instance.new('LocalScript', TextButton_13)

		script.Parent.MouseButton1Click:Connect(function()
			fkick = not fkick
		end)
	end
	coroutine.wrap(GDOZSA_fake_script)()
	local function ELZGOHJ_fake_script() -- TextButton_14.LocalScript 
		local script = Instance.new('LocalScript', TextButton_14)

		script.Parent.MouseButton1Click:Connect(function()
			antikick = not antikick
		end)
	end
	coroutine.wrap(ELZGOHJ_fake_script)()
	local function YKBCN_fake_script() -- TextButton_15.LocalScript 
		local script = Instance.new('LocalScript', TextButton_15)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";moveresetpad")
		end)
	end
	coroutine.wrap(YKBCN_fake_script)()
	local function BKYKUQ_fake_script() -- TextButton_16.LocalScript 
		local script = Instance.new('LocalScript', TextButton_16)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";moveobbybox")
		end)
	end
	coroutine.wrap(BKYKUQ_fake_script)()
	local function GQMOGSE_fake_script() -- TextButton_17.LocalScript 
		local script = Instance.new('LocalScript', TextButton_17)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";moveobbybricks")
		end)
	end
	coroutine.wrap(GQMOGSE_fake_script)()
	local function ADENMM_fake_script() -- TextButton_18.LocalScript 
		local script = Instance.new('LocalScript', TextButton_18)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";moveadmindividers")
		end)
	end
	coroutine.wrap(ADENMM_fake_script)()
	local function ETQNRT_fake_script() -- TextButton_19.LocalScript 
		local script = Instance.new('LocalScript', TextButton_19)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";movepads")
		end)
	end
	coroutine.wrap(ETQNRT_fake_script)()
	local function KYQBY_fake_script() -- TextButton_20.LocalScript 
		local script = Instance.new('LocalScript', TextButton_20)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";movebuildingbricks")
		end)
	end
	coroutine.wrap(KYQBY_fake_script)()
	local function ISOUZ_fake_script() -- TextButton_21.LocalScript 
		local script = Instance.new('LocalScript', TextButton_21)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";movehouse")
		end)
	end
	coroutine.wrap(ISOUZ_fake_script)()
	local function SJGAVR_fake_script() -- TextButton_22.LocalScript 
		local script = Instance.new('LocalScript', TextButton_22)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";color all original")
		end)
	end
	coroutine.wrap(SJGAVR_fake_script)()
	local function UCAS_fake_script() -- TextButton_23.LocalScript 
		local script = Instance.new('LocalScript', TextButton_23)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";color all random")
		end)
	end
	coroutine.wrap(UCAS_fake_script)()
	local function KJYI_fake_script() -- TextButton_24.LocalScript 
		local script = Instance.new('LocalScript', TextButton_24)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";shutdown")
		end)
	end
	coroutine.wrap(KJYI_fake_script)()
	local function BNPJUSH_fake_script() -- TextButton_25.LocalScript 
		local script = Instance.new('LocalScript', TextButton_25)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";sch")
		end)
	end
	coroutine.wrap(BNPJUSH_fake_script)()
	local function ADLSJYY_fake_script() -- TextButton_26.LocalScript 
		local script = Instance.new('LocalScript', TextButton_26)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";upme")
		end)
	end
	coroutine.wrap(ADLSJYY_fake_script)()
	local function IWCDYQI_fake_script() -- TextButton_27.LocalScript 
		local script = Instance.new('LocalScript', TextButton_27)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";pads")
		end)
	end
	coroutine.wrap(IWCDYQI_fake_script)()
	local function MSZYQ_fake_script() -- TextButton_28.LocalScript 
		local script = Instance.new('LocalScript', TextButton_28)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";house")
		end)
	end
	coroutine.wrap(MSZYQ_fake_script)()
	local function FOQZRI_fake_script() -- TextButton_29.LocalScript 
		local script = Instance.new('LocalScript', TextButton_29)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";spawn")
		end)
	end
	coroutine.wrap(FOQZRI_fake_script)()
	local function HKLR_fake_script() -- TextButton_30.LocalScript 
		local script = Instance.new('LocalScript', TextButton_30)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";findresetpad")
		end)
	end
	coroutine.wrap(HKLR_fake_script)()
	local function ZUIBX_fake_script() -- TextButton_31.LocalScript 
		local script = Instance.new('LocalScript', TextButton_31)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";ohnana")
		end)
	end
	coroutine.wrap(ZUIBX_fake_script)()
	local function SWOULII_fake_script() -- TextButton_32.LocalScript 
		local script = Instance.new('LocalScript', TextButton_32)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";movebaseplate")
		end)
	end
	coroutine.wrap(SWOULII_fake_script)()
	local function OREYPMV_fake_script() -- TextButton_33.LocalScript 
		local script = Instance.new('LocalScript', TextButton_33)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";infjump")
		end)
	end
	coroutine.wrap(OREYPMV_fake_script)()
	local function GVHNRE_fake_script() -- TextButton_34.LocalScript 
		local script = Instance.new('LocalScript', TextButton_34)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";allpads")
		end)
	end
	coroutine.wrap(GVHNRE_fake_script)()
	local function KJQR_fake_script() -- TextButton_35.LocalScript 
		local script = Instance.new('LocalScript', TextButton_35)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";lagall")
		end)
	end
	coroutine.wrap(KJQR_fake_script)()
	local function REANZLF_fake_script() -- TextButton_36.LocalScript 
		local script = Instance.new('LocalScript', TextButton_36)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";stop")
		end)
	end
	coroutine.wrap(REANZLF_fake_script)()
	local function PMEORY_fake_script() -- TextButton_37.LocalScript 
		local script = Instance.new('LocalScript', TextButton_37)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";rej")
		end)
	end
	coroutine.wrap(PMEORY_fake_script)()
	local function SHJXI_fake_script() -- TextButton_38.LocalScript 
		local script = Instance.new('LocalScript', TextButton_38)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";reg")
		end)
	end
	coroutine.wrap(SHJXI_fake_script)()
	local function OOUQRE_fake_script() -- TextButton_39.LocalScript 
		local script = Instance.new('LocalScript', TextButton_39)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";cmds")
		end)
	end
	coroutine.wrap(OOUQRE_fake_script)()
	local function NEHHA_fake_script() -- TextButton_40.LocalScript 
		local script = Instance.new('LocalScript', TextButton_40)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";alltools")
		end)
	end
	coroutine.wrap(NEHHA_fake_script)()
	local function ONUHYA_fake_script() -- TextButton_41.LocalScript 
		local script = Instance.new('LocalScript', TextButton_41)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";traplogs")
		end)
	end
	coroutine.wrap(ONUHYA_fake_script)()
	local function GMOJIZ_fake_script() -- TextButton_42.LocalScript 
		local script = Instance.new('LocalScript', TextButton_42)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";disablerc")
		end)
	end
	coroutine.wrap(GMOJIZ_fake_script)()
	local function NHZOTC_fake_script() -- TextButton_43.LocalScript 
		local script = Instance.new('LocalScript', TextButton_43)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";ds")
		end)
	end
	coroutine.wrap(NHZOTC_fake_script)()
	local function ITMH_fake_script() -- TextButton_44.LocalScript 
		local script = Instance.new('LocalScript', TextButton_44)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";clicktp")
		end)
	end
	coroutine.wrap(ITMH_fake_script)()
	local function TTLALKA_fake_script() -- TextButton_45.LocalScript 
		local script = Instance.new('LocalScript', TextButton_45)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("reset me")
		end)
	end
	coroutine.wrap(TTLALKA_fake_script)()
	local function FIHMFUG_fake_script() -- TextButton_46.LocalScript 
		local script = Instance.new('LocalScript', TextButton_46)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("fly me")
		end)
	end
	coroutine.wrap(FIHMFUG_fake_script)()
	local function HZKLHFZ_fake_script() -- TextButton_47.LocalScript 
		local script = Instance.new('LocalScript', TextButton_47)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("ff me")
		end)
	end
	coroutine.wrap(HZKLHFZ_fake_script)()
	local function JZHWCDC_fake_script() -- TextButton_48.LocalScript 
		local script = Instance.new('LocalScript', TextButton_48)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("god me")
		end)
	end
	coroutine.wrap(JZHWCDC_fake_script)()
	local function GNUWRVW_fake_script() -- TextButton_49.LocalScript 
		local script = Instance.new('LocalScript', TextButton_49)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("respawn me")
		end)
	end
	coroutine.wrap(GNUWRVW_fake_script)()
	local function XGTLXX_fake_script() -- TextButton_50.LocalScript 
		local script = Instance.new('LocalScript', TextButton_50)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat(";heykindle me")
		end)
	end
	coroutine.wrap(XGTLXX_fake_script)()
	local function MAKRZ_fake_script() -- TextButton_51.LocalScript 
		local script = Instance.new('LocalScript', TextButton_51)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("kill me")
		end)
	end
	coroutine.wrap(MAKRZ_fake_script)()
	local function SWUQDRL_fake_script() -- TextButton_52.LocalScript 
		local script = Instance.new('LocalScript', TextButton_52)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("jail me")
		end)
	end
	coroutine.wrap(SWUQDRL_fake_script)()
	local function NRGR_fake_script() -- TextButton_53.LocalScript 
		local script = Instance.new('LocalScript', TextButton_53)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("unjail me")
		end)
	end
	coroutine.wrap(NRGR_fake_script)()
	local function WZVWGO_fake_script() -- TextButton_54.LocalScript 
		local script = Instance.new('LocalScript', TextButton_54)

		script.Parent.MouseButton1Click:Connect(function()
			game.Players:Chat("speed me 16")
		end)
	end
	coroutine.wrap(WZVWGO_fake_script)()
	local function PPPFA_fake_script() -- TextButton_55.LocalScript 
		local script = Instance.new('LocalScript', TextButton_55)

		script.Parent.MouseButton1Click:Connect(function()
			loadstring(script.Parent.Parent.TextBox.Text)()
		end)
	end
	coroutine.wrap(PPPFA_fake_script)()
	local function NVRSPFO_fake_script() -- TextButton_56.LocalScript 
		local script = Instance.new('LocalScript', TextButton_56)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent.TextBox.Text = ""
		end)
	end
	coroutine.wrap(NVRSPFO_fake_script)()
	local function NRXI_fake_script() -- Page1.LocalScript 
		local script = Instance.new('LocalScript', Page1)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent.Parent.WelcomePage.Visible = true
			script.Parent.Parent.Parent.TogglesPage.Visible = false
			script.Parent.Parent.Parent.ShortcutsPage.Visible = false
			script.Parent.Parent.Parent.SettingsPage.Visible = false
		end)
	end
	coroutine.wrap(NRXI_fake_script)()
	local function JRKZ_fake_script() -- Page2.LocalScript 
		local script = Instance.new('LocalScript', Page2)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent.Parent.WelcomePage.Visible = false
			script.Parent.Parent.Parent.TogglesPage.Visible = true
			script.Parent.Parent.Parent.ShortcutsPage.Visible = false
			script.Parent.Parent.Parent.SettingsPage.Visible = false
		end)
	end
	coroutine.wrap(JRKZ_fake_script)()
	local function GBEK_fake_script() -- Page3.LocalScript 
		local script = Instance.new('LocalScript', Page3)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent.Parent.WelcomePage.Visible = false
			script.Parent.Parent.Parent.TogglesPage.Visible = false
			script.Parent.Parent.Parent.ShortcutsPage.Visible = true
			script.Parent.Parent.Parent.SettingsPage.Visible = false
		end)
	end
	coroutine.wrap(GBEK_fake_script)()
	local function ZAWZV_fake_script() -- Page4.LocalScript 
		local script = Instance.new('LocalScript', Page4)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent.Parent.WelcomePage.Visible = false
			script.Parent.Parent.Parent.TogglesPage.Visible = false
			script.Parent.Parent.Parent.ShortcutsPage.Visible = false
			script.Parent.Parent.Parent.SettingsPage.Visible = true
		end)
	end
	coroutine.wrap(ZAWZV_fake_script)()
	local function PXSI_fake_script() -- HideTabs.LocalScript 
		local script = Instance.new('LocalScript', HideTabs)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent["HideTabs <"].Visible = true
			script.Parent.Parent["HideTabs >"].Visible = false
			script.Parent.Parent.TabsBar.Visible = true
		end)
	end
	coroutine.wrap(PXSI_fake_script)()
	local function MJFDHG_fake_script() -- HideTabs_2.LocalScript 
		local script = Instance.new('LocalScript', HideTabs_2)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Parent["HideTabs <"].Visible = false
			script.Parent.Parent["HideTabs >"].Visible = true
			script.Parent.Parent.TabsBar.Visible = false
		end)
	end
	coroutine.wrap(MJFDHG_fake_script)()
	local function CNGZ_fake_script() -- ReopenShortcut.LocalScript 
		local script = Instance.new('LocalScript', ReopenShortcut)

		script.Parent.MouseButton1Click:Connect(function()
			script.Parent.Visible = false
			script.Parent.Parent.Frame.Visible = true
		end)
	end
	coroutine.wrap(CNGZ_fake_script)()

end)

Spawn(function()
	mods()
end)

logn("Code fully executed!")

wait(0.25)

Spawn(function()
	startupScripts()
end)