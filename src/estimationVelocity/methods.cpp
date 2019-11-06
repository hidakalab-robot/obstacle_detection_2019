﻿#include<obstacle_detection_2019/velocityEstimation.h>

//subscribe
void velocityEstimation::cluster_callback(const obstacle_detection_2019::ClassificationVelocityData::ConstPtr& msg)
{
    //データをコピー
	curClstr = *msg;
	//move manage method
	manage();
}
void velocityEstimation::manage(){
	
	ROS_INFO("if(!isPreCluster())");
	if(!isPreCluster()){
		ROS_INFO("First process");
		renewMessages();
		return ;
	}
	ROS_INFO("kalmanFilter");
	kalmanFilter();
	//
	ROS_INFO("recordTwistData");
	recordTwistData();
	ROS_INFO("averageFilter");
	averageFilter();
	ROS_INFO("decisionObstacleType");
	decisionObstacleType();
	ROS_INFO("publishData");
	publishData();
	ROS_INFO("debug");
	debug();
	ROS_INFO("renewMessages");
	renewMessages();
}
bool velocityEstimation::isPreCluster(){
	if((int)preClstr.data.size() > 0){
		return true;
	}
	return false;
}
void velocityEstimation::kalmanFilter(){//
	//カルマンフィルタ後クラスタデータの初期化
	filtedClstr = curClstr;
	//define
	ROS_INFO("define");
	Eigen::MatrixXd K_t(4,4);
	Eigen::MatrixXd xt_t(4,1);
	Eigen::MatrixXd z_t(4,1);
	Eigen::MatrixXd F_t = Eigen::MatrixXd::Zero(4,4);
	Eigen::MatrixXd B_t = Eigen::MatrixXd::Zero(2,2);
	Eigen::MatrixXd sig_xt(4,4);
	Eigen::MatrixXd u_t(4,1);
	//resize
	// xh_t_1.resize(xh_t.size());
	// sig_xh_t_1.resize(sig_xh_t.size());
	// xh_t_1.resize(curClstr.data.size());
	// sig_xh_t_1.resize(curClstr.data.size());

	float dt = curClstr.dt;
	ROS_INFO("initialize");
	//initialize
	//F
	F_t(0,0)=1;
	F_t(1,1)=1;
	F_t(2,2)=1;
	F_t(3,3)=1;
	F_t(0,2)=dt;
	F_t(1,3)=dt;
	//
	// for(int i=0;i<curClstr.data.size();i++)
	// {
	// 	z_t(0,0)=curClstr.data[i].gc.x;
	// 	z_t(1,0)=curClstr.data[i].gc.y;
	// 	z_t(2,0)=curClstr.twist[i].linear.x;
	// 	z_t(3,0)=curClstr.twist[i].linear.y;
	// 	//can't do filter
	// 	if(curClstr.trackingNum[i]<2)//1:no tracking,2:vel
	// 	{
	// 		// xh_t_1[i]=z_t;//観測データを推定結果としてそのまま出力
	// 		// sig_xh_t_1[i]=sig_x0;//推定値の共分散は初期分散
	// 		continue;
	// 	}
	// 	ROS_INFO("curClstr.match[%d]: %d", i, curClstr.match[i]);
	// 	ROS_INFO("xh_t_1[i].size(), xh_t.size(): %d, %d", (int)xh_t_1.size(), (int)xh_t.size());
	// 	//推定位置速度、共分散を更新
	// 	xh_t_1[i]=xh_t[ curClstr.match[i] ];
	// 	sig_xh_t_1[i]=sig_xh_t[ curClstr.match[i] ];
	// }
	//
	ROS_INFO("update data");
	//データの更新
	// ROS_INFO("(int)xh_t.size():%d", (int)xh_t.size());
	if((int)xh_t.size() > 0){//データあり
		xh_t_1.clear();
		sig_xh_t_1.clear();
		xh_t_1.resize(xh_t.size());
		sig_xh_t_1.resize(sig_xh_t.size());
		//古いデータの更新
		for(int i=0;i<xh_t.size();i++){
			xh_t_1[i]=xh_t[i];
			sig_xh_t_1[i]=sig_xh_t[i];
		}
	}
	else{//データ無し（初回処理）
		// ROS_INFO("(int)preClstr.data.size():%d", (int)preClstr.data.size());
	 	//過去の観測データを推定結果として格納
		xh_t_1.resize(preClstr.data.size());
		sig_xh_t_1.resize(preClstr.data.size());
		for(int i=0;i<preClstr.data.size();i++){
			//過去の観測データ
			//軸反転あり
			// z_t(0,0)=preClstr.data[i].gc.y;
			// z_t(1,0)=preClstr.data[i].gc.x;
			// z_t(2,0)=preClstr.twist[i].linear.y;
			// z_t(3,0)=preClstr.twist[i].linear.x;
			//軸反転なし
			z_t(0,0)=preClstr.data[i].gc.x;
			z_t(1,0)=preClstr.data[i].gc.y;
			z_t(2,0)=preClstr.twist[i].linear.x;
			z_t(3,0)=preClstr.twist[i].linear.y;
			xh_t_1[i] = z_t;//過去の観測データを推定結果としてそのまま出力
			sig_xh_t_1[i]=sig_x0;//推定値の共分散は初期分散
			//
		}
	}

	//kalman filter
	ROS_INFO("process");
	//resize
	xh_t.resize(curClstr.data.size());
	sig_xh_t.resize(curClstr.data.size());
	//process
	for(int i=0;i<curClstr.data.size();i++)
	{
		//set z
		//軸反転あり
		// z_t(0,0)=preClstr.data[i].gc.y;
		// z_t(1,0)=preClstr.data[i].gc.x;
		// z_t(2,0)=preClstr.twist[i].linear.y;
		// z_t(3,0)=preClstr.twist[i].linear.x;
		//軸反転なし
		// z_t(0,0)=preClstr.data[i].gc.x;
		// z_t(1,0)=preClstr.data[i].gc.y;
		// z_t(2,0)=preClstr.twist[i].linear.x;
		// z_t(3,0)=preClstr.twist[i].linear.y;
		// std::cout<<"z_t,gc.x:"<<curClstr.data[i].gc.x<<std::endl;
		// std::cout<<"z_t,gc.y:"<<curClstr.data[i].gc.y<<std::endl;
		// std::cout<<"z_t,linear.x:"<<curClstr.twist[i].linear.x<<std::endl;
		// std::cout<<"z_t,linear.y:"<<curClstr.twist[i].linear.y<<std::endl;
		z_t(0,0)=curClstr.data[i].gc.x;
		z_t(1,0)=curClstr.data[i].gc.y;
		z_t(2,0)=curClstr.twist[i].linear.x;
		z_t(3,0)=curClstr.twist[i].linear.y;
		float dt = curClstr.dt;
		//set ut
		if(curClstr.match[i] < 0){//マッチングなし
			// ROS_INFO("no matching[%d]",i);
			// std::cout<<z_t<<std::endl;
			// xh_t[i] = z_t;//過去の観測データを推定結果としてそのまま出力
			xh_t[i] = z_t;//現在の観測データを推定結果としてそのまま出力
			sig_xh_t[i]=sig_x0;//推定値の共分散は初期分散		
		}
		else{//マッチングあり
			//軸反転あり
			// u_t(0,0)=preClstr.twist[ curClstr.match[i] ].linear.y;//vel[i].x;
			// u_t(1,0)=preClstr.twist[ curClstr.match[i] ].linear.x;//vel[i].z;
			//軸反転なし
			u_t(0,0)=preClstr.twist[ curClstr.match[i] ].linear.x;//vel[i].x;
			u_t(1,0)=preClstr.twist[ curClstr.match[i] ].linear.y;//vel[i].z;
			//
			//カルマンフィルタ更新ステップ
			xt_t = F_t*xh_t_1[ curClstr.match[i] ];// + B_t*u_t ;
			sig_xt = F_t * sig_xh_t_1[ curClstr.match[i] ] * F_t.transpose()+sig_wk;// + B_t * sig_ut * B_t.transpose() ;
			K_t = sig_xt*( (sig_xt+del_t).inverse() );
			xh_t[i] = xt_t + K_t*( z_t - xt_t );
			sig_xh_t[i] = (I - K_t)*sig_xt;
		}
		//推定後データの格納
		filtedClstr.twist[i].linear.x = xh_t[i](2,0);
		filtedClstr.twist[i].linear.y = xh_t[i](3,0);
	}
}
void velocityEstimation::decisionObstacleType(){
	//移動障害物かどうかを判断
	//静止障害物->速度ゼロ
	//移動障害物->推定速度
	float cellSize = filtedClstr.res.data * filtedClstr.res.data;
	for(int k=0; k<filtedClstr.data.size(); k++){
		//判断
		std::cout<<"size "<< k << " = "<< filtedClstr.data[k].size.data << " * " << cellSize << std::endl;
		if( std::sqrt(sig_xh_t[k](2, 2) + sig_xh_t[k](3, 3) ) 
            > std::sqrt(std::pow(filtedClstr.twist[k].linear.x, 2.0) + std::pow(filtedClstr.twist[k].linear.y, 2.0))
            || std::sqrt(std::pow(filtedClstr.twist[k].linear.x, 2.0) + std::pow(filtedClstr.twist[k].linear.y, 2.0)) >= velMaxThreshold
            || std::sqrt(std::pow(filtedClstr.twist[k].linear.x, 2.0) + std::pow(filtedClstr.twist[k].linear.y, 2.0)) <= velMinThreshold
			|| filtedClstr.data[k].size.data * cellSize < sizeMinThreshold
			|| filtedClstr.data[k].size.data * cellSize > sizeMaxThreshold
			|| filtedClstr.trackingNum[k] < trackThreshold){
				filtedClstr.twist[k].linear.x = 0;
				filtedClstr.twist[k].linear.y = 0;
				filtedClstr.twist[k].linear.z = 0;
        }
        else{
			//no process
        }
		//障害物の状態を確認
		std::cout<<"\t gc:"<< filtedClstr.data[k].gc.x <<","<<filtedClstr.data[k].gc.y << std::endl;
		std::cout<<"\t twist:"<< filtedClstr.twist[k].linear.x <<","<<filtedClstr.twist[k].linear.y << std::endl;
		std::cout<<"\t trackingNum:"<< filtedClstr.trackingNum[k] << std::endl;
		
	}
}
//平均フィルタ
void velocityEstimation::averageFilter(){
	//average filter
	for(int k=0; k<record_twists.size();k++){
		ROS_INFO("record_twists[%d].n = %d",k,record_twists[k].n);
		if(record_twists[k].n == filterN){
			record_twists[k].sum_twist = record_twists[k].twistArray[0];
			for(int i=1;i<record_twists[k].n; i++){
				record_twists[k].sum_twist.linear.x = record_twists[k].sum_twist.linear.x + record_twists[k].twistArray[i].linear.x;
				record_twists[k].sum_twist.linear.y = record_twists[k].sum_twist.linear.y + record_twists[k].twistArray[i].linear.y;
				record_twists[k].sum_twist.linear.z = record_twists[k].sum_twist.linear.z + record_twists[k].twistArray[i].linear.z;
			}
			//平均を求め
			filtedClstr.twist[k].linear.x = record_twists[k].sum_twist.linear.x / filterN;//反転 
			filtedClstr.twist[k].linear.y = record_twists[k].sum_twist.linear.y / filterN; 
			filtedClstr.twist[k].linear.z = record_twists[k].sum_twist.linear.z / filterN; 
		}
	}
}
void velocityEstimation::recordTwistData(){

	std::vector<record_twist> record_twists_temp;
	record_twists_temp.resize(filtedClstr.data.size() );

	if((int)record_twists.size() <= 0){
		//記録データが皆無
		//完全に初期処理
		//推定データをそのまま確保する
		record_twists_temp.resize(filtedClstr.data.size());
		for(int k=0; k<filtedClstr.data.size();k++){
			record_twists_temp[k].n = 1;
			record_twists_temp[k].twistArray.resize(1);
			record_twists_temp[k].twistArray[0] =  filtedClstr.twist[k];
		}
	}
	else{
		//データがある時
		//処理回数２回以上
		for(int k=0; k<filtedClstr.data.size();k++){
			int prev_n =filtedClstr.match[k];
			//データマッチングが取れない
			//障害物を見失ったとき
			if(prev_n == -1){
				//データ追加(初期データ)
				record_twists_temp[k].n = 1;
				record_twists_temp[k].twistArray.resize(1);
				record_twists_temp[k].twistArray[0] = filtedClstr.twist[k];
			}
			else{
				//追跡中の障害物
				//記録中データに取得データを追加する
				//取得データ
				record_twists_temp[k].n = 1;
				record_twists_temp[k].twistArray.resize(1);
				record_twists_temp[k].twistArray[0] = filtedClstr.twist[k];
				//記録データを加算
				record_twists_temp[k].n += record_twists[prev_n].n;
				record_twists_temp[k].twistArray.reserve(filterN+1);
				std::copy(record_twists[prev_n].twistArray.begin(),record_twists[prev_n].twistArray.end(),std::back_inserter(record_twists_temp[k].twistArray));
				//サイズ超過時
				while(record_twists_temp[k].n > filterN){
					//一番後ろの要素を削除
					record_twists_temp[k].twistArray.pop_back();
					record_twists_temp[k].n -= 1;
				}
			}
		}
	}
	record_twists = record_twists_temp;
}
void velocityEstimation::publishData(){//データ送信

	//データ修正 (x軸が反転しているのを修正) 
	for(int k=0; k<filtedClstr.twist.size();k++){
		filtedClstr.twist[k].linear.x = - filtedClstr.twist[k].linear.x;//反転 
		filtedClstr.twist[k].linear.y = filtedClstr.twist[k].linear.y; 
		filtedClstr.twist[k].linear.z = filtedClstr.twist[k].linear.z; 
	}
	//データ修正 (x軸が反転しているのを修正) 
	for(int k=0; k<filtedClstr.twist.size();k++){
		filtedClstr.data[k].gc.x = - filtedClstr.data[k].gc.x;//反転 
		filtedClstr.data[k].gc.y = filtedClstr.data[k].gc.y; 
		filtedClstr.data[k].gc.z = filtedClstr.data[k].gc.z; 
	}
	//データパブリッシュ
    pub.publish(filtedClstr);
}
void velocityEstimation::renewMessages(){
	preClstr = curClstr;
}