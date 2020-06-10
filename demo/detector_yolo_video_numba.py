import time
import numpy as np
import cv2
import numba as nb

@nb.jit()
def postprocessing(outs,shape):
    # Postprocessing
    frameHeight = shape[0]
    frameWidth = shape[1]
    # Scan through all the bounding boxes output from the network and keep only the
    # ones with high confidence scores. Assign the box's class label as the class with the highest score.
    classIds = np.zeros((0,1),np.int)
    confidences = np.zeros((0,1),np.float)
    boxes = np.zeros((0,4),np.int)
    for out in outs:
        for detection in out:
            scores = detection[5:]
            classId = np.argmax(scores)
            confidence = scores[classId]
            if confidence > confThreshold:
                center_x = int(detection[0] * frameWidth)
                center_y = int(detection[1] * frameHeight)
                width = int(detection[2] * frameWidth)
                height = int(detection[3] * frameHeight)
                left = int(center_x - width / 2)
                top = int(center_y - height / 2)
                classIds = np.append(classIds,classId)
                confidences = np.append(confidences,float(confidence))
                boxes = np.append(boxes,[[left,top,width,height]],axis=0)

    # Perform non maximum suppression to eliminate redundant overlapping boxes with
    # lower confidences.
    if len(boxes) == 0:
        indices = np.array([],np.int)
    elif len(boxes) == 1:
        indices = np.array([[0]], np.int)
    else:
        indices = cv2.dnn.NMSBoxes(boxes, confidences, confThreshold, nmsThreshold)
    final_classids = np.array([])
    final_boxes = np.zeros((0,4),np.int)
    for i in indices:
        final_classids = np.append(final_classids,classIds[i[0]])
        final_boxes = np.append(final_boxes,[boxes[i[0]]],axis=0)

    return final_classids, final_boxes

# Initialize the parameters
confThreshold = 0.5  # Confidence threshold
nmsThreshold = 0.4   # Non-maximum suppression threshold
inpWidth = 320       # Width of network's input image
inpHeight = 320      # Height of network's input image

# Give the configuration and weight files for the model and load the network using them.
modelConfiguration = "darknet-yolov3.cfg"
modelWeights = "darknet-yolov3_last.weights"

# load model
net = cv2.dnn.readNetFromDarknet(modelConfiguration, modelWeights)
#net.setPreferableBackend(cv2.dnn.DNN_BACKEND_OPENCV)
#net.setPreferableTarget(cv2.dnn.DNN_TARGET_CPU) # 1-2 fps
net.setPreferableBackend(cv2.dnn.DNN_BACKEND_CUDA)
net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA) # 4 fps
#net.setPreferableTarget(cv2.dnn.DNN_TARGET_CUDA_FP16) # supported only by the newest GPU
print('model loaded')

# label of the followed object
label = 0

# open video
video = cv2.VideoCapture('1586356900.avi')

fps = 0
last_fps = 0
last_sec = int(time.time())
while True:
    hasFrame, frame = video.read()
    if not hasFrame:
        break
    
    tt0 = time.time()    
    # Create a 4D blob from a frame.
    blob = cv2.dnn.blobFromImage(frame, 1/255, (inpWidth, inpHeight), [0,0,0], 1, crop=False)

    # Sets the input to the network
    t0 = time.time()
    net.setInput(blob)

    # Runs the forward pass to get output of the output layers
    outs = net.forward(['yolo_82', 'yolo_94', 'yolo_106'])
    t = time.time() - t0

    # Postprocessing
    classids, boxes = postprocessing(outs,frame.shape)
    
    result = np.copy(frame)
    for i in range(boxes.shape[0]):
        classid = classids[i]
        box = boxes[i]
        left = box[0]
        top = box[1]
        width = box[2]
        height = box[3]
        if classid == label:
            cv2.rectangle(result, (left, top), (left+width, top+height), (0,0,255), 2)
    
    tt = time.time() - tt0
    cv2.putText(result, str(t), (16, 32), 0, 1.0, (0, 0, 255), 2)
    cv2.putText(result, str(tt), (16, 64), 0, 1.0, (0, 0, 255), 2)
    cv2.putText(result, str(last_fps), (16, 96), 0, 1.0, (0, 255, 0), 2)
    cv2.imshow('camera',result)
    key = cv2.waitKey(1)
    if key == 27:
        break
    elif key == ord('s'):
        cv2.imwrite('recorded-numba.png',result)

    sec = int(time.time())
    if sec == last_sec:
        fps += 1
    else:
        last_fps = fps + 1
        fps = 0
    last_sec = sec
    #print('sec',sec,'fps',fps)

cv2.destroyAllWindows()
