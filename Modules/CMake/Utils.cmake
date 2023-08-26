# Copyright Notices: [...]

# Replace '\' with '\\'
function(REPLACE_BACKSLASHES_IN_VARIABLE INPUT_VARIABLE)
    string(REPLACE "\\" "\\\\" ${INPUT_VARIABLE} "${${INPUT_VARIABLE}}")
endfunction()
