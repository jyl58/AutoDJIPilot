--split the str use split char 
function stringSplit(str,split_char)
	local sub_table={};
	while(true) do
		local pos=string.find(str,split_char);
		if (not pos) then
			sub_table[#sub_table+1]=str;
			break;
		end
		local sub_str=string.sub(str,1,pos-1);
		sub_table[#sub_table+1]=sub_str;
		str=string.sub(str,pos+1,#str);
	end
	return sub_table;
end

