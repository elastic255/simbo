classdef rede <handle
    %UNTITLED Summary of this class goes here
    %   Detailed explanation goes here
    
    properties
        snifferdir = 'D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\felipe\Simulation\terceiro\io\sniffer\';
        nos = no;
        maxid;
        maxnos = 1000;
        lasttime;
        time;
    end
    
    methods
%--------------------------------------        
        function obj = rede()
            obj.nos(obj.maxnos) = no; 
            if nargin == 0
                obj.carregaSnifferNos;
            end
        end
%--------------------------------------
        function carregaSnifferNos(obj)
            lista_diretorio =  dir(obj.snifferdir);
            id = 1;
            for i = 1:length(lista_diretorio)
                if(lista_diretorio(i).isdir)
                    continue;
                end
                nomehost = lista_diretorio(i).name;
                pathhost = strcat(obj.snifferdir,nomehost);
                obj.nos(id) = no(pathhost,nomehost);
                id = id+1;
            end
            obj.maxid = id-1;
        end
%--------------------------------------  
function origemPacotesRede(obj)
    
    for i = 1:obj.maxid
        subplot(ceil(obj.maxid/4),4,i);
        obj.nos(i).origemPacotesBar();
    end
end
%--------------------------------------  
    end
    
end

