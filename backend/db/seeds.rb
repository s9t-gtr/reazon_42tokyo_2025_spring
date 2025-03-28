# 固定データの挿入
Session.create!(
  date: Date.new(2025, 3, 25),
  duration: 120.5,
  heltzh: 60.2,
  yaw_up: 45.0,
  pitch_left: 30.5,
  pressure_avg: 1012.3,
  overpressure_count: 5,
  total_vibration: 60.2 * 120.5 # heltzh * duration
)

Session.create!(
  date: Date.new(2025, 3, 24),
  duration: 90.0,
  heltzh: 50.0,
  yaw_up: 40.0,
  pitch_left: 20.0,
  pressure_avg: 1010.5,
  overpressure_count: 3,
  total_vibration: 50.0 * 90.0 # heltzh * duration
)

Session.create!(
  date: Date.new(2025, 3, 20),
  duration: 110.0,
  heltzh: 70.0,
  yaw_up: 50.0,
  pitch_left: 25.0,
  pressure_avg: 1015.0,
  overpressure_count: 2,
  total_vibration: 70.0 * 110.0 # heltzh * duration
)

# ランダムデータ生成関数
def generate_random_session
  date = Faker::Date.between(from: '2025-01-01', to: '2025-03-28')
  duration = rand(50..250) # 50秒から250秒の範囲
  heltzh = rand(40..100) # 40から100の範囲
  yaw_up = rand(0..90)   # 0から90の範囲
  pitch_left = rand(0..90) # 0から90の範囲
  pressure_avg = rand(1000..1050) # 1000から1050の範囲
  overpressure_count = rand(0..10) # 0から10の範囲
  total_vibration = heltzh * duration # heltzh * durationで計算

  Session.create!(
    date: date,
    duration: duration,
    heltzh: heltzh,
    yaw_up: yaw_up,
    pitch_left: pitch_left,
    pressure_avg: pressure_avg,
    overpressure_count: overpressure_count,
    total_vibration: total_vibration # total_vibrationを追加
  )
end

# 10個のランダムデータを生成
60.times { generate_random_session }
