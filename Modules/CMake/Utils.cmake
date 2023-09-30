# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# Replace '\' with '\\'
function(REPLACE_BACKSLASHES_IN_VARIABLE INPUT_VARIABLE)
    string(REPLACE "\\" "\\\\" ${INPUT_VARIABLE} "${${INPUT_VARIABLE}}")
endfunction()
