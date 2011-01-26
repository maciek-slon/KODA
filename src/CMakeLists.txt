# Create an executable file from sources

SET(LIBRARY_OUTPUT_PATH    ${CMAKE_BINARY_DIR}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# Find all required packages
FIND_PACKAGE( Boost REQUIRED program_options)

INCLUDE_DIRECTORIES(${BOOST_INCLUDEDIR})

# Find OpenCV library files
FIND_PACKAGE( OpenCV REQUIRED )

ADD_EXECUTABLE(enchuf enchuf.cpp)
ADD_EXECUTABLE(dechuf dechuf.cpp)

#ADD_EXECUTABLE(split split.cpp)
#TARGET_LINK_LIBRARIES(split ${OpenCV_LIBS})

#ADD_EXECUTABLE(bayer_split bayer_split.cpp)
#TARGET_LINK_LIBRARIES(bayer_split ${OpenCV_LIBS})

#ADD_EXECUTABLE(bitsplit bitsplit.cpp)
#TARGET_LINK_LIBRARIES(bitsplit ${OpenCV_LIBS})

#ADD_EXECUTABLE(nkb2gray nkb2gray.cpp)
#TARGET_LINK_LIBRARIES(nkb2gray ${OpenCV_LIBS})

#ADD_EXECUTABLE(rle rle.cpp)
#TARGET_LINK_LIBRARIES(rle ${OpenCV_LIBS})

ADD_EXECUTABLE(codec codec.cpp)
TARGET_LINK_LIBRARIES(codec ${OpenCV_LIBS} ${Boost_PROGRAM_OPTIONS_LIBRARY})

#ADD_EXECUTABLE(analyze analyze.cpp)
