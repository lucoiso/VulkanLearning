# Author: Lucas Vilas-Boas
# Year: 2023
# Repo: https://github.com/lucoiso/VulkanRender

# Replace '\' with '\\'
FUNCTION(REPLACE_BACKSLASHES_IN_VARIABLE INPUT_VARIABLE)
	STRING(REPLACE "\\" "\\\\" ${INPUT_VARIABLE} "${${INPUT_VARIABLE}}")
ENDFUNCTION()
