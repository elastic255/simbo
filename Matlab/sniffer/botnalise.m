classdef botnalise < handle
    %UNTITLED3 Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        filename = 'D:\desenvolvimento\omnet\omnetpp-5.0\samples\Jteste12\simulations\terceiro\new.txt';
        dest;
        origem;
        ips;
        meuip = 167772161 ;
        nomes;
    end
    
methods
%-------------------------------------------------------    
function obj = botnalise(path)
    if nargin == 1
        obj.filename = path;
    end
end  

%-------------------------------------------------------
function [ out ] = origemPacotesQueEntraram(obj)
A = obj.ips(:,1);
A = cell2mat(A');
B = obj.ips(:,2);
C = categorical(obj.origem(obj.dest==obj.meuip),A,B);
pie(C);
title('Origem dos pacotes que entraram:')
end
%-------------------------------------------------------
function [ out ] = getdados(obj )
A = importdata(obj.filename);
B = A.textdata;
obj.nomes = B(:,1);
text_dest = B(:,2);
text_origem = B(:,3);
obj.dest = cellfun(@obj.ip2int,text_dest);
obj.origem = cellfun(@obj.ip2int,text_origem);
ip = num2cell(unique([obj.dest,obj.origem]));
ip2 = cellfun(@obj.int2ip,ip,'UniformOutput',false);
obj.ips = [ip,ip2];
out = obj.ips;
end
%-------------------------------------------------------
function [ ip ] = ip2int(~,in )
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
%-------------------------------------------------------
function [ str ] = int2ip(~,in )
    
    if iscell(in)
        A = int64(in{1});
    else
        A = int64(in);
    end
    B = int64([0,0,0,0]);
    for i=1:4
        B(i) = idivide( A, int64(256^(4-i)), 'floor');
        A = A - B(i)*int64(256^(4-i));
        
    end
    str = [num2str(B(1)) '.' num2str(B(2)) '.' num2str(B(3)) '.' num2str(B(4))];
end
%-------------------------------------------------------        
        
        
    end %end methods
    
end %end class

