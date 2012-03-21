SerialPort='com7'; % serial port
N=1000;

m=zeros(1,1000);
figure(1)
hLine = plot(m);
ylim([0 1024]);

KeepRunning = 1;
while KeepRunning
    s = serial(SerialPort);
    set(s,'BaudRate',57600);
    fopen(s);
    for i = 1:N
        datum = fscanf(s, '%s');
        %fprintf('%s\n', datum);

        if (length(datum) > 0)
            m(i) = str2num(datum);
        else
            m(i) = 0;
        end

        if (ishandle(1))
            set(hLine,'YData',m);
        else
            KeepRunning = 0;
            break;
        end
        drawnow
    end
    
    % Clean up the serial port
    fclose(s);
    delete(s);
    clear s;
end