# ROM DYNAMICS AMR
## rom_msgs ( meta-package )
### အဆင့် ( ၂ ) Navigation from QT ( Traditional method )
#### 1. service client from qt xxxxxxxxxxxx to edit
```
# ၁။ ပထမဆုံး qt မှာ 
/qt_construct_yaml_client node 
# သည် ui app မှ Save Waypoints button ကို နှိပ်ပါက 
/construct_yaml 
# service name ဖြင့် service client တစ်ခုဖန်တီးကာ 
rom_interfaces::srv::ConstructYaml 
# Data type ဖြင့် waypoints များပါဝင်သော  list ကို server server သို့ပို့ပါမယ်။
```

#### 2. service server ရဲ့ အလုပ် ( construct_yaml_server.cpp)
```
# ၁။ သူက 
/construct_yaml_server_bt node 
# ဖြစ်ပြီး
/home/mr_robot/Desktop/Git/rom_dynamics_robots/developer_packages/rom2109/rom2109_bt/trees/rom_trees/default.xml
# ဖိုင်ကို write လုပ်ပေးသလို
/waypoints_list 
# topic ကို transient local နဲ့ publish လုပ်ပေးပါတယ်။ အခြား host ပေါ်က qt app များက WaypointListSubscriber object ဖြင့်  subscribe ပြုလုပ်ပြီး waypoint list ကို ui မှာ ပြန်လည် ပြနိုင်ဖို့ဖြစ်ပါတယ်။
```
#### 3. Send Goals
```
# အခုဆိုရင် waypoint များကို save ပြီးပြီဖြစ်ပြီး qt app မှာ Goals button ကို loops check/uncheck ဖြင့် နှိပ်ပါက 
/qt_send_wp_client
# ဆိုတဲ့ node သည် 
/waypoints_selected
ဆိုတဲ့ service name ဖြင့် 
rom_interfaces::srv::ConstructYaml
# service type ကို ထပ်မံအသုံးပြုပြီး နောက်ဆုံးနှစ်လုံးဖြုတ်ကာ သွားရမည့် waypoint name များကို ထပ်မံပို့ပေးပါတယ်။ နောက်ဆုံး element ကို loop ( true/false ), ဒုတိယနောက်ဆုံး element ကို command အနေဖြင့် အသုံးပြုထားတာမို့ pop() ပြန်လုပ်ပေးတာဖြစ်ပါတယ်။
```
#### 4. Receive Goals
```
send_waypoints_server.cpp 
# သည် 
/waypoint_server
# node အမည်ဖြင့် service request များကို ရယူပြီး
send_waypoints_all_goals.launch.py
send_waypoints_custom_goals.launch.py
send_waypoints_all_goals_loop.launch.py
send_waypoints_custom_goals_loop.launch.py
# launch ဖိုင် ၄ ခုမှ တစ်ခုကို launch လုပ်ပေးပါမယ်။
# အဲ့ဒီမှာ custom goals နဲ့ loop true case များအတွက် implement မလုပ်ရသေးပါ။ ထို့ အပြင် /waypoint_server သည် 
/navigation_stop
# အမည်ဖြင့် publisher တစ်ခု ပါဝင်ပြီး navigation ကို ရပ်ရန်ရည်ရွယ်သည်။ implement မလုပ်ရသေးပါ။
```
