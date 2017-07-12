tic
cd D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto;
x = zeros(1:10);
y = char(zeros(10,200));
parfor i = 1:20
disp(i)    
[x, y] = dos(strcat('D:\desenvolvimento\omnet\omnetpp-5.0\samples\inet\src\inet\applications\simbo\Simulation\Quinto\run_bala-Console-ini1.bat  >> teste',int2str(i),'.txt'));

end
toc