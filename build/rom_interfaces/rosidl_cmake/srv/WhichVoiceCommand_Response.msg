

# Response (Gemini/Service ဘက်က ပြန်ပေးမည့်အချက်အလက်)
bool success                # Service အောင်မြင်သလား
string transcription        # User ပြောလိုက်တဲ့ စကားလုံး (Speech-to-Text)
string response_text        # Gemini ပြန်ပြောတဲ့ စကား (Speaker ကနေ ထွက်လာမယ့်အသံအတွက်)
string action_type          # ဘာလုပ်ရမလဲ? (ဥပမာ: "navigate", "find_object", "chat", "stop")
string action_params        # Action အတွက် လိုအပ်သော Parameter များ (JSON format ဖြင့်)
string error_msg            # အမှားအယွင်းရှိလျှင် ပြပေးရန်

