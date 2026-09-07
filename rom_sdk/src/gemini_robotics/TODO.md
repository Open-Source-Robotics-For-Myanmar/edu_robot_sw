python နဲ့ gemini robotics ကို api ကနေ ခိုင်းမည့် code ရေးရမယ်။ 

environment ထဲက api key ရယူမယ်။ 

ငါလိုတဲ့ function တွေက subscribeCamera1() , speakVoiceResponse()# to speaker, 

which_voice_command ဆိုတဲ့ ROS2 Service server တစ်ခု # Wav file က ဒီကို  ရောက်ပါမယ်။ 
service types က rom_interface/srv/WhichVoiceCommand.srv ဖြစ်ပါတယ်။

nav2_simple_commander ကို ခိုင်းမယ့် client function တစ်ခု 

Algorithm: 

1. QT ကနေ Voice Command service received ရတာနဲ့ 

- နေရာ တစ်နေရာကို သွားခိုင်းတဲ့ကိစ္စဆိုရင် /home/buc_robot/data/waypoints/ ထဲက yaml file အားလုံးနဲ့ wav file ကို gemini စီကိုပို့မယ်။ 

Gemini က ပြန်ပေးရမှာက blablabla

2. 
- တစ်ခုခုကို ရှာခိုင်းတဲ့ကိစ္စဆိုရင် subscribeCamera1() ကို ခေါ်ပြီး Camera Data နဲ့ voice command ကို gemini ကိုပို့မယ်။

3. 
- object တစ်ခုကို ရှာပြီးသွားခိုင်းတဲ့ကိစ္စဆိုရင် subscribeCamera1() ကို ခေါ်ပြီး Camera Data ကိုယူပြီး object detection လုပ်ပြီး voice command ကို gemini ကိုပို့မယ်။ 
object ကိုမတွေ့မချင်း robot ကို 360 degree မပြည့်မချင်း ဖြည်းဖြည်းလှည့်ပြီး image frame ကို gemini ကိုပို့မယ်။ 
-  Gemini က object တွေ့တာနဲ့ ရပ်ခိုင်းပြီး user စီကို speakVoiceResponse() ကို ခေါ်ပြီး "I found the object you were looking for!" ဆိုပြီး ပြောမယ်။