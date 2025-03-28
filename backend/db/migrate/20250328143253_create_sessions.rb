class CreateSessions < ActiveRecord::Migration[8.0]
  def change
    create_table :sessions do |t|
      t.date :date
      t.float :duration
      t.float :heltzh
      t.float :yaw_up
      t.float :pitch_left
      t.float :pressure_avg
      t.integer :overpressure_count

      t.timestamps
    end
  end
end
