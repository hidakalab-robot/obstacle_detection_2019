#include <obstacle_detection_2019/darknetImg.h>

void darknetImg::trimPoints(darknet_ros_msgs::BoundingBoxes& bbs){
    std::vector<bool> checkFlag;
    int groupNumber = 0;
    checkFlag.resize(bbs.bounding_boxes.size());
    std::fill(checkFlag.begin(), checkFlag.end(), false);
    for(int i = 0; i < checkFlag.size(); ++i){
        if(!checkFlag[i]){
            ++groupNumber;
            addBBGroupRecursively(bbs, checkFlag, i, groupNumber);
        }
    }
}

void darknetImg::addBBGroupRecursively(darknet_ros_msgs::BoundingBoxes& bbs, std::vector<bool>& checkFlag, int coreNumber, int groupNumber){
    int searchNumber = 0;
    checkFlag[coreNumber] = true;
    for(searchNumber = 0; searchNumber < checkFlag.size(); ++searchNumber){
        if(!checkFlag[searchNumber]){
            switch (checkBoundingBoxesRelationship(bbs, coreNumber, searchNumber)) { //他のBBとの関連を調べる
                case darknetImg::Relationship::MIX: //一部含まれている場合は同groupであり、探査BBをcoreにして再度関連を調べる -> 探査済みにする
                    addBBGroupRecursively(bbs, checkFlag, searchNumber, groupNumber);
                    break;
                case darknetImg::Relationship::NONE: //含まれていない場合は別groupであるため、何もしない
                    break;
            }
        }
    }
    drawMask(bbs, coreNumber, (char)groupNumber, mask);
}

darknetImg::Relationship darknetImg::checkBoundingBoxesRelationship(darknet_ros_msgs::BoundingBoxes& bbs, int index_1, int index_2){
    int center_1_x = (bbs.bounding_boxes[index_1].xmax + bbs.bounding_boxes[index_1].xmin) / 2;
    int center_1_y = (bbs.bounding_boxes[index_1].ymax + bbs.bounding_boxes[index_1].ymin) / 2;
    int center_2_x = (bbs.bounding_boxes[index_2].xmax + bbs.bounding_boxes[index_2].xmin) / 2;
    int center_2_y = (bbs.bounding_boxes[index_2].ymax + bbs.bounding_boxes[index_2].ymin) / 2;
    int halfSide_1_w = (bbs.bounding_boxes[index_1].xmax - bbs.bounding_boxes[index_1].xmin) / 2;
    int halfSide_1_h = (bbs.bounding_boxes[index_1].ymax - bbs.bounding_boxes[index_1].ymin) / 2;
    int halfSide_2_w = (bbs.bounding_boxes[index_2].xmax - bbs.bounding_boxes[index_2].xmin) / 2;
    int halfSide_2_h = (bbs.bounding_boxes[index_2].ymax - bbs.bounding_boxes[index_2].ymin) / 2;

    if(   halfSide_1_w + halfSide_2_w < abs(center_1_x - center_2_x) 
       || halfSide_1_h + halfSide_2_h < abs(center_1_y - center_2_y)
    ){
        return darknetImg::Relationship::NONE;
        // ROS_INFO_STREAM("match");
    }
    return darknetImg::Relationship::MIX;
}

void darknetImg::drawMask(darknet_ros_msgs::BoundingBoxes& bbs, int target_indexs, char value,  cv::Mat& mask){
    int row_min = bbs.bounding_boxes[target_indexs].ymin;
    int row_max = bbs.bounding_boxes[target_indexs].ymax;
    int col_min = bbs.bounding_boxes[target_indexs].xmin;
    int col_max = bbs.bounding_boxes[target_indexs].xmax;
    for(int row = row_min; row < row_max; ++row){
        auto *p = mask.ptr<char>(row) + col_min;
        for(int col = col_min; col < col_max; col++){
            *p = value * 50;
            ++p;
        }
    }
    ROS_INFO_STREAM("min[" << col_min << ", " << row_min << "], max[" << col_max << ", " << row_max << "]");
}

//trimPoints()旧バージョン:廃止
// void darknetImg::trimPoints(){
//     int i = 1;
//     for(const auto& bb : boundingBoxesMsg.bounding_boxes){
//         cv::rectangle(mask, cv::Point(bb.xmin, bb.ymin), cv::Point(bb.xmax, bb.ymax), cv::Scalar(i,0,0), 1, CV_FILLED, 0);
//         ++i;
//     }
// }
