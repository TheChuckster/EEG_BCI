% settings
SerialPort='com3'; % serial port
N=200;
Fs=200;

m=zeros(1,N);

s = serial(SerialPort);
set(s,'BaudRate',57600);
fopen(s);

for i = 1:N
    datum = fscanf(s, '%s');
    fprintf('%s\n', datum);

    if (length(datum) > 0)
        m(i) = str2num(datum);
    else
        m(i) = 0;
    end
end

% Clean up the serial port
fclose(s);
delete(s);
clear s;

% Filter m with 60 Hz notch
Wo = 60/(Fs/2);  BW = Wo/35;
[b,a] = iirnotch(Wo,BW);
m = filter(b,a,m);

% Remove DC offset
mu = mean(m);
m = m - mu + 1024/2;

figure(1)
hLine = plot(m);
ylim([0 1024]);
set(hLine,'YData',m);

figure(2)
L=length(m);
NFFT = 2^nextpow2(L); % Next power of 2 from length of y
Y = fft(m,NFFT)/L;
f = Fs/2*linspace(0,1,NFFT/2+1);

% Plot single-sided amplitude spectrum.
plot(f,2*abs(Y(1:NFFT/2+1))) 
title('Single-Sided Amplitude Spectrum of y(t)')
xlabel('Frequency (Hz)')
ylabel('|Y(f)|')