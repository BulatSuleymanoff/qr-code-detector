# qr-code-detector

Algorithm:
Geometric detection:
- Convert to gray image
- Binarization
- Canny Edge Detector
- Find contours, where one or more contours are inside another (1 square in angle of QR code)
- Find 3 squares like this one
- Choose orientation (there may be 4 different orientations, but only one is correct)
- Perform affine transformation to get perspective view of QR code

Color detection (if geometric detection does not work for the original image):
- Erosion
- Convert to gray image
- Use only black part of the erosed gray image
- Find contours of black part of the image
- Create polygon that is near to contour of the black part of the image
- Check if the polygon is square (not only a rectangle)
- Create image with white or black background and the part of original image that corresponds to 
  black part of processed image
- Perform geometric detection
