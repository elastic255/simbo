mylogicalarray = ~cellfun(@isempty,mycellarray);  %# Já serve.
emptyIndex = cellfun(@isempty,mycellarray);       %# Find indices of empty cells
mycellarray(emptyIndex) = {0};                    %# Fill empty cells with 0
mylogicalarray = logical(cell2mat(mycellarray));  %# Convert the cell array