AUX_SOURCE_DIRECTORY(. SRCS_FILES)       # 这个表示获取当前src目录下所有文件名并存入SRCS_FILES这个变量中
ADD_LIBRARY(SRCS ${SRCS_FILES})         # 这个表示将${SRCS_FILES}这些文件加入库中，然后并且复制一份到SRCS这个变量中