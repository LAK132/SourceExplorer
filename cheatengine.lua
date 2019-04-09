-- game_dir = "C:\\GamesDir\\"
-- game_exe = "GameName.exe"
-- game_data_offset = 0xAC9AC

createProcess(game_dir .. game_exe, "", true, false)

game_data_ptr = getAddress(game_exe .. "+" .. string.format("%X", game_data_offset))
print("Game data ptr: " .. string.format("%X", game_data_ptr))

game_data_address = 0
while game_data_address == 0 do
    game_data_address = readInteger(game_data_ptr)
end
print("Game data address: " .. string.format("%X", game_data_address))
print("Header: " .. readString(game_data_address, 4, false))
decode_key_address = game_data_address + 0x26C
print("Decode key address: " .. string.format("%X", decode_key_address))