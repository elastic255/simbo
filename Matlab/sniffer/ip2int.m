function [ ip ] = ip2int( in )
%UNTITLED3 Summary of this function goes here
%   Detailed explanation goes here
if iscell(in)
    str = in{1};
else
    str = in;
end
C = strsplit(str,'.');
ip = int64(0);
    for i=1:4
        ip = ip+str2num(C{i})*(256^(4-i));
    end

end

