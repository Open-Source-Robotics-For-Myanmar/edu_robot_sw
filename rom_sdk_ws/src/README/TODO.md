# ROM DYNAMICS AMR
## rom_dymamics_app


## ===================================   TODO   
```

### 77. charging point ထည့်ရန်, ဘယ်လိုထည့်မလဲ? တခါထဲ table စာရင်းပါဆွဲရန်

### 88. screen size အပြည့် ဒါမှမဟုတ် အားလုံး ညီတူဖြစ်စေရန်

### 99. map change ရင် carto, navi တို့ restart လုပ်လား/မလုပ်လား စစ်ရန်
restart လုပ်ပုံရတယ်။ localization ကြာတယ်။

### 100. MainWindow::updateUI() မှာ
```
showSceneOriginCoordinate(), showMapOriginCooridinate(), updateWpList() တို့ကို odom, base_footprint, robot တို့လို pointer type ပြောင်းရန်
```



## ===================================   DONE   =========================================
### 0. Application Core dumped, crashed ပြသနာ
```ပြသနာနေရာများမှာ class တွေမှာ mutex ဖန်တီးပြီး lock_guard လုပ်ဖို့ ကြိုးစားကြည့်ပါ။```

### 1. rviz, open vim ( DONE )


### 2. user setting yaml parser implement လုပ်ရန် ( DONE )


### 3. မြေပုံ flip ပြသနာ ( DONE )
#### 3.1 Line numbers 2176, 2185 တို့မှာ ( DONE )
```
mapImage.setPixel(x, y, color.rgb());
// ဒါကို အောက်ကကောင်နဲ့ အစားထိုးပေးပါ။

int inverted_y = msg->info.height - 1 - y;
mapImage.setPixel(x, inverted_y, color.rgb());
```
#### 3.2 Line numbers 761, 912, 1031, 2886( လိုမလို မသိသေး) တို့မှာ ( DONE )
```
double mapX = (scenePoint.x() * this->map_resolution_) + this->map_origin_x_;
double mapY = ((scene_height - scenePoint.y()) * this->map_resolution_) + this->map_origin_y_;
// ထည့်ပေးပါ။
``` 


### 4. မြေပုံ flip ဖြစ်သွားတာကြောင့် laser, ပြန် စစ်ရန် ( DONE )
```
laser -> line number 3025
int map_height_pixels_ = this->map_height_;

double _x = ( ( ( robot_pose_x_ + x ) - map_origin_x_) / map_resolution_ );
double _y = map_height_pixels_ - ( ( ( robot_pose_y_ + y ) - map_origin_y_) / map_resolution_ );

#3090
double _x = ( ( ( robot_pose_x_ + x ) - map_origin_x_) / map_resolution_ );
double _y = map_height_pixels_ - ( ( ( robot_pose_y_ + y ) - map_origin_y_) / map_resolution_ );
```


### 5. မြေပုံ flip ဖြစ်သွားတာကြောင့် robot_pose ပြန်စစ်ရန် ( Done )
```
robot pose --> 3415
arrowItem->setRotation(-yaw_deg+90.0);
#3327
double sceneY_map = map_height_ - ((rom_tf_.odom_base_footprint_y - this->map_origin_y_) / this->map_resolution_);
```


### 6. မြေပုံ flip ဖြစ်သွားတာကြောင့် laser, odom, map မှန်/မမှန် ပြန် စစ်ရန် ( Done )


### 7. waypoints circle size မတူတဲ့ ပြသနာ  ( Done )
```
Line 3525 က calculateScaledRadius() ဖန်ရှင်ကို  အသုံးပြုပြီး
```
#### 7.1 wps subscriber လုပ်ရာမှာ wp, service, patrol ပုံစံအတိုင်း ပြန်လာရန် color အနည်းငယ်ကွဲပြီးပြရန်  ( DONE ) 
```
HOST မှာပဲ RUN မှာမို့ မလိုတော့လို့ ဖြုတ်လိုက်တယ်။
```
### 7.3 robot server မှ publish လုပ်သော pose များလိုမလို ဘယ်အချိန်လိုမလဲ ( DONE )
```
ထင်တာက pose ကို ဖေါ်ပြဖို့အတွက်ပဲသုံးပါ။ yaml, bt များက ရှိပြီးသားမို့ အဆင်ပြေတယ်လို့ထင်တယ်။
patrol လား wp လား service လား  အဆင့်လိုက်ပြပါ။
7.1 မလိုတော့လို့ 7.3 လည်းမလိုတော့ပါ။
```


### 8. မြေပုံပါပါ မပါပါ app core dumped ဖြစ်သွားခြင်း ( DONE )
```
if else နဲ့ ခနဖြေရှင်းထားတယ်။
```

### 9. shutdown button update ( DONE )

### 10. build with debug ( DONE )
```
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
# ပြီးရင် qtcreator ကို ros2 pkg များတွေ့အောင် terminal မှ ဖွင့်ပါ။
# CMakeLists.txt ကိုရွေးပါ။
# ဘယ်ဘက် Panel မှ Project ကို နှိပ်ပြီး Build မှာ
-DCMAKE_BUILD_TYPE:STRING=Build အား 
-DCMAKE_BUILD_TYPE:STRING=Debug ပြောင်းပေးပါ။
```

### 11. mapping နဲ့ nav ပြန်လာရင် robot နဲ့ လေဆာပေါ်ဖို့ ( DONE )
```
scan, odom, base_footprint, robot တို့ကို pointer type များနဲ့ဖြေရှင်းပြီး 
```
