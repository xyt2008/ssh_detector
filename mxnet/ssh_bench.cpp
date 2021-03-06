#include "ssh_detector_mxnet.h"

#include <stdlib.h>
#include <sys/time.h>

#include <stdio.h>

static float getElapse(struct timeval *tv1,struct timeval *tv2)
{
    float t = 0.0f;
    if (tv1->tv_sec == tv2->tv_sec)
        t = (tv2->tv_usec - tv1->tv_usec)/1000.0f;
    else
        t = ((tv2->tv_sec - tv1->tv_sec) * 1000 * 1000 + tv2->tv_usec - tv1->tv_usec)/1000.0f;
    return t;
}

double tenengrad(const cv::Mat& src, int ksize)
{
    cv::Mat Gx, Gy;
    cv::Sobel(src, Gx, CV_64F, 1, 0, ksize);
    cv::Sobel(src, Gy, CV_64F, 0, 1, ksize);

    cv::Mat FM = Gx.mul(Gx) + Gy.mul(Gy);

    double focusMeasure = cv::mean(FM).val[0];
    return focusMeasure;
}

bool getEyeRoi(cv::Mat img, cv::Point2f eye_point, cv::Mat & res) {
    cv::Point2f p1(eye_point.x-5,eye_point.y-5);
    cv::Rect2f r1(p1.x,p1.y,10,10);
    cv::Rect2f rect_all(0,0,img.cols,img.rows);
    cv::Rect2f r2 = r1 & rect_all;
    if(r2.area()<=0) return false;
    res = img(r2);
    return true;
}

int main(int argc, char* argv[]) {

  const std::string keys =
      "{help h usage ? |                | print this message   }"
      "{model_path     |../../model     | path to ssh model  }"
      "{model_name     |mneti           | model name }"
      "{blur           |false           | if use blur scores }" 
      "{peroid         |1              | detect peroid }"
      "{threshold      |0.95            | threshold for detect score }"
      "{nms_threshold  |0.3             | nms_threshold for ssh detector }"      
      "{output         |./output        | path to save detect output  }"
      "{input          |../../../video  | path to input video file  }"      
      "{dynamic        |false           | true if input size is dynamic  }"
      "{@video         |camera-244-crop-8p.mov     | input video file }"
  ;

  cv::CommandLineParser parser(argc, argv, keys);
  parser.about("ssh detector benchmark");
  if (parser.has("help")) {
      parser.printMessage();
      return 0;
  }

  if (!parser.check()) {
      parser.printErrors();
      return EXIT_FAILURE;
  }

  std::string model_path = parser.get<std::string>("model_path");
  std::string model_name = parser.get<std::string>("model_name");
  bool blur = parser.get<bool>("blur");
  int peroid              = parser.get<int>("peroid");
  float threshold         = parser.get<float>("threshold");
  float nms_threshold     = parser.get<float>("nms_threshold");
  std::string output_path = parser.get<std::string>("output");
  std::string input_path  = parser.get<std::string>("input");
  std::string video_file  = parser.get<std::string>(0);
  bool dynamic = parser.get<bool>("dynamic");
  std::string video_path  = input_path + "/" + video_file;

  std::string output_folder = output_path + "/" + video_file;
  std::string cmd = "rm -rf " + output_folder;
  system(cmd.c_str());
  cmd = "mkdir -p " + output_folder;
  system(cmd.c_str());

  cv::VideoCapture capture(video_path);
  cv::Mat frame_, frame;
  int frame_count = 1;
  capture >> frame;
  if(!frame.data) {
      std::cout<< "read first frame failed!";
      exit(1);
  }
  SSH * det = new SSH(model_path, model_name, threshold, nms_threshold, blur);

  std::cout << "frame resolution: " << frame.cols << "*" << frame.rows << "\n";

  std::vector<cv::Rect2f> boxes;
  std::vector<cv::Point2f> landmarks;
  std::vector<float> scores;
  std::vector<float> blur_scores;  

  struct timeval  tv1,tv2;

  while(1){
      capture >> frame_;
      /*
      float resize_rate = 1;
      resize_rate += (rand() % 10) / 10.0;
      cv::Size s = frame_.size();
      s.width  /= resize_rate;
      s.height /= resize_rate;
      cv::resize(frame_,frame,s);
      */
      if(!frame_.data)
        break;
      int w = frame_.cols;
      int h = frame_.rows;
      w *=  (rand() % 20 + 1 ) / 20.0;
      h *=  (rand() % 20 + 1 ) / 20.0;
      int x = rand() % frame_.cols;
      int y = rand() % frame_.rows;
      cv::Rect roi(x,y,w,h);
      roi = roi & cv::Rect(0,0,frame_.cols,frame_.rows);
      
      if(dynamic)
        frame = cv::Mat(frame_, roi);
      else
          frame = frame_;

      frame_count++;
      if(frame_count%peroid!=0) continue;
      
      gettimeofday(&tv1,NULL);
      if(blur)
        det->detect(frame,boxes,landmarks,scores,blur_scores);
      else
        det->detect(frame,boxes,landmarks,scores);
      gettimeofday(&tv2,NULL);
      std::cout << "detected one frame, time eclipsed: " <<  getElapse(&tv1, &tv2) << " ms\n";

      for(auto & b: boxes)
        cv::rectangle( frame, b, cv::Scalar( 255, 0, 0 ), 2, 1 );
        
      for(int i=0;i<boxes.size();i++){
        cv::rectangle( frame, boxes[i], cv::Scalar( 255, 0, 0 ), 2, 1 );

        // cv::circle(frame,landmarks[i*5], 5, cv::Scalar( 255, 0, 0 ), 2, 1);
        // cv::circle(frame,landmarks[i*5+1], 5, cv::Scalar( 255, 0, 0 ), 2, 1);
        /*
        cv::Mat e1,e2;
        double s1,s2;
        bool res = getEyeRoi(frame, landmarks[i*5], e1);
        if(res) 
            s1 = tenengrad(e1,3);
        else 
            s1 = 0;
        res = getEyeRoi(frame, landmarks[i*5+1],e2);
        if(res) 
            s2 = tenengrad(e1,3);
        else 
            s2 =0;
        
        double s = 0;
        cv::Rect2f rect_ = boxes[i] & cv::Rect2f(0,0,frame.cols,frame.rows);
        if(rect_.area()>0)  {
            cv::Mat box = frame(boxes[i]);
            s = tenengrad(box,7);
        }
        */
        // s = s / 10000;
        cv::Point middleHighPoint = cv::Point(boxes[i].x+boxes[i].width/2, boxes[i].y);
        char buf_text[64];
        if(blur)
            snprintf(buf_text, 64, "s:%.3f, b: %.3f", scores[i], blur_scores[i]);
        else
            snprintf(buf_text, 64, "s:%.3e", scores[i]);
        std::string text(buf_text);
        cv::putText(frame, text, middleHighPoint, cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);

      }

      //for(auto & p: landmarks)
      //  cv::drawMarker(frame, p,  cv::Scalar(0, 255, 0), cv::MARKER_CROSS, 10, 1);    

      if(boxes.size()>0)
        cv::imwrite( output_folder + "/" + std::to_string(frame_count) + ".jpg", frame);

  }

  delete det;

}