target_sources(VisionFlowNodes PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/dnn_image_classification.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_object_detection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_segmentation.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_human_pose.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_text_detection.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_text_recognition.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_image_processing.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_backend_helper.cpp
    ${CMAKE_CURRENT_LIST_DIR}/dnn_human_pose_helper.cpp
)
