classdef no <handle
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        ipdestino;
        iporigem;
        pdestino;
        porigem;
        tag;
        bytesize;
        time;
        ips;
        
        txtip;
        meuip;
        
    end
    
    methods
%--------------------------------------        
function obj = no(path,nomehost)
    if nargin >= 1
        obj.carregaSnifferHost(path);
    end
    if nargin >= 2
        a = strsplit(nomehost,'.txt');
        obj.txtip = a(1);
        obj.meuip = ip2int(obj.txtip);
    end
end
%--------------------------------------
function carregaSnifferHost(obj,path)
    A = importdata(path);
    B = A.textdata;
    obj.tag = B(:,1);
    %ips----------------------
    text_dest = B(:,2);
    text_origem = B(:,3);
    obj.ipdestino = cellfun(@ip2int,text_dest);
    obj.iporigem = cellfun(@ip2int,text_origem);

    ip = num2cell(unique([obj.ipdestino,obj.iporigem]));
    ip2 = cellfun(@int2ip,ip,'UniformOutput',false);
    obj.ips = [ip,ip2];

    %----------------------
    obj.pdestino = A.data(:,1);
    obj.porigem = A.data(:,2);
    obj.bytesize = A.data(:,3);
    obj.time = A.data(:,4);
end
%-------------------------------------------------------
function origemPacotesPie(obj, pt)
    t = obj.time;
    A = obj.ips(:,1);
    A = cell2mat(A');
    B = obj.ips(:,2);
    
    if nargin < 2
        C = categorical(obj.iporigem(obj.ipdestino==obj.meuip),A,B);
    else
        C = categorical(obj.iporigem(obj.ipdestino==obj.meuip & t>=pt*120 & t< pt*120+120),A,B);
    end
    
    if isempty(C)
        C = [0 2 4 6; 8 10 12 14; 16 18 20 22];
        image(C)
        pause(0.001)
    else
        pie(C);
        title(obj.txtip);
        pause(0.001)
    end
    %t = strcat('Origem dos pacotes destinados ao host:',obj.txtip);
    
end
%-------------------------------------------------------
function origemPacotesBar(obj)
    j = 0;
    t = length(obj.ips);
    x = double(t);
    y = char(t,8);
    for i = 1:t      
        if(obj.ips{i,1} == obj.meuip)
            continue;
        end
        j = j+1;
        a = obj.ipdestino == obj.meuip;
        x(j) = sum(a(:));
        y(j,1:8) = obj.ips{i,2};
        
    end
    
    bar(x);
    set(gca, 'XTick', 1:j, 'XTickLabel', y);
    title(obj.txtip);
    
end
%-------------------------------------------------------


    end
    
end

