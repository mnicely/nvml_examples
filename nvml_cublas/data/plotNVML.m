%% Make plots
read_gpu_stats('gpuStats.csv')

function read_gpu_stats( file )
    % Read in data into matlab table
    T = readtable(file, 'Delimiter', ',', 'ReadVariableNames',true);
    
    % Get filename
    specs = extractBefore(file, '.');
    
    % Clean Header
    for i = 1:size(T,2)
        str = split(T.Properties.VariableNames{i}, '_');
        T.Properties.VariableNames(i) = join(upper(str(~cellfun('isempty', str))), '_');
    end
    
    %% Plots
    set(0, 'DefaultFigureVisible', 'off');
    sgtitle("GPU Stats : NVML Library");
    lineWidth = 1;
    
    % Temperature
    subplot(4,1,1)
    plot(T.TEMPERATURE_GPU, 'b', "LineWidth", lineWidth);
    xlim([0 length(T.TEMPERATURE_GPU)])
    ylim([0 80])
    
    title('Temperature')
    xlabel('Samples')
    ylabel('Temperature (C)')
    legend('Temperature', 'Location', 'southoutside', "Orientation", "horizontal")
    legend('boxoff')
    grid on
    
    % Power
    subplot(4,1,2)
    hold on
    plot(T.POWER_DRAW_W, 'b', "LineWidth", lineWidth)
    plot(T.POWER_LIMIT_W, 'r', "LineWidth", lineWidth);
    ylim([0 260])
    yticks([0 65 130 195 260])
    ylabel('Watts (W)')
    hold off
    
    yyaxis right
    plot(T.PSTATE, 'g', "LineWidth", lineWidth);
    ylim([0 8])
    yticks([0 2 4 6 8])
    ylabel('State')
    
    xlim([0 length(T.TEMPERATURE_GPU)])
    
    title('Power Draw')
    xlabel('Samples')
    legend('Draw', 'Threshold', 'PState', 'Location', 'southoutside', "Orientation", "horizontal")
    legend('boxoff')
    grid on
    
    % Clocks
    subplot(4,1,3)
    hold on
    plot(T.CLOCKS_CURRENT_SM_MHZ, 'b', "LineWidth", lineWidth)
    plot(T.CLOCKS_APPLICATIONS_GRAPHICS_MHZ, 'r', "LineWidth", lineWidth);
    ylim([0 1600])
    yticks([0 400 800 1200 1600])
    ylabel('Frequency (MHz)')
    hold off
    
    yyaxis right
    plot(T.CLOCKS_THROTTLE_REASONS_ACTIVE, 'g', "LineWidth", lineWidth);
    ylim([0 8])
    yticks([0 2 4 6 8])
    ylabel('Throttle Reason')
    
    xlim([0 length(T.TEMPERATURE_GPU)])
    
    title('SM Clock Frequency')
    xlabel('Samples')
    legend('Current', 'Threshold', 'Throttle Reason', 'Location', 'southoutside', "Orientation", "horizontal")
    legend('boxoff')
    grid on
    
    % Utilization
    subplot(4,1,4)
    hold on
    plot(T.UTILIZATION_GPU, 'b', "LineWidth", lineWidth)
    plot(T.UTILIZATION_MEMORY, 'r', "LineWidth", lineWidth);
    xlim([0 length(T.TEMPERATURE_GPU)])
    hold off
    
    title('Utilization')
    xlabel('Samples')
    ylabel('Percentage (%)')
    legend('GPU', 'Memory', 'Location', 'southoutside', "Orientation", "horizontal")
    legend('boxoff')
    grid on

    % Print
    print(specs,'-dpdf','-fillpage');
    
    % Reset workspace
    clear; clc; close all;
end