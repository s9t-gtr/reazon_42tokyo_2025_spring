import React, { useState, useRef, useEffect } from "react";
import { Play, Square, Plus, Trash2, Volume2, Upload } from "lucide-react";

interface Track {
  id: string;
  buffer: AudioBuffer | null;
  volume: number;
  playbackRate: number; // BPM調整用
}

function App() {
  const [tracks, setTracks] = useState<Track[]>([]);
  const [isPlaying, setIsPlaying] = useState(false);

  const audioContext = useRef<AudioContext | null>(null);

  // Initialize audio context
  useEffect(() => {
    audioContext.current = new AudioContext();
    return () => {
      audioContext.current?.close();
    };
  }, []);

  // MP3ファイルのアップロード処理
  const handleFileUpload = async (
    event: React.ChangeEvent<HTMLInputElement>
  ) => {
    const file = event.target.files?.[0];
    if (!file) return;

    const arrayBuffer = await file.arrayBuffer();
    const audioBuffer =
      await audioContext.current!.decodeAudioData(arrayBuffer);

    setTracks((prev) => [
      ...prev,
      {
        id: Date.now().toString(),
        buffer: audioBuffer,
        volume: 1,
        playbackRate: 1,
      },
    ]);
  };

  // トラックを再生
  const playTracks = () => {
    if (!audioContext.current || tracks.length === 0) return;

    setIsPlaying(true);
    const startTime = audioContext.current.currentTime;

    tracks.forEach((track) => {
      if (!track.buffer) return;

      const source = audioContext.current!.createBufferSource();
      const gainNode = audioContext.current!.createGain();

      source.buffer = track.buffer;
      source.playbackRate.value = track.playbackRate; // BPM調整
      gainNode.gain.value = track.volume;

      source.connect(gainNode);
      gainNode.connect(audioContext.current!.destination);

      // ループ再生設定
      source.loop = true;

      source.start(startTime);
    });
  };

  // 再生停止
  const stopPlayback = () => {
    if (!audioContext.current) return;
    audioContext.current.close();
    audioContext.current = new AudioContext();
    setIsPlaying(false);
  };

  // トラックの削除
  const deleteTrack = (id: string) => {
    setTracks((prev) => prev.filter((track) => track.id !== id));
  };

  // 音量調整
  const updateTrackVolume = (id: string, volume: number) => {
    setTracks((prev) =>
      prev.map((track) => (track.id === id ? { ...track, volume } : track))
    );
  };

  // BPM調整
  const updateTrackBPM = (id: string, playbackRate: number) => {
    setTracks((prev) =>
      prev.map((track) =>
        track.id === id ? { ...track, playbackRate } : track
      )
    );
  };

  // トラックが更新された時に自動的に再生する
  useEffect(() => {
    if (tracks.length > 0 && !isPlaying) {
      playTracks();
    }
  }, [tracks]); // トラックが変更されるたびに再生を開始

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-900 to-indigo-900 p-8">
      <div className="max-w-4xl mx-auto">
        <h1 className="text-4xl font-bold text-white mb-8 text-center">
          React Audio BPM Looper
        </h1>

        <div className="bg-white/10 backdrop-blur-lg rounded-lg p-6 mb-8">
          <div className="flex justify-center gap-4 mb-8">
            <input
              type="file"
              accept="audio/mp3"
              onChange={handleFileUpload}
              className="hidden"
              id="file-upload"
            />
            <label
              htmlFor="file-upload"
              className="flex items-center gap-2 bg-blue-500 hover:bg-blue-600 text-white px-6 py-3 rounded-full transition-colors cursor-pointer"
            >
              <Upload size={20} />
              Upload MP3
            </label>

            {!isPlaying ? (
              <button
                onClick={playTracks}
                disabled={tracks.length === 0}
                className="flex items-center gap-2 bg-green-500 hover:bg-green-600 disabled:bg-gray-400 text-white px-6 py-3 rounded-full transition-colors"
              >
                <Play size={20} />
                Play All
              </button>
            ) : (
              <button
                onClick={stopPlayback}
                className="flex items-center gap-2 bg-yellow-500 hover:bg-yellow-600 text-white px-6 py-3 rounded-full transition-colors"
              >
                <Square size={20} />
                Stop
              </button>
            )}
          </div>

          <div className="space-y-4">
            {tracks.map((track, index) => (
              <div
                key={track.id}
                className="flex items-center gap-4 bg-white/5 p-4 rounded-lg"
              >
                <span className="text-white font-medium">
                  Track {index + 1}
                </span>
                <div className="flex-1 flex items-center gap-4">
                  <Volume2 size={20} className="text-white" />
                  <input
                    type="range"
                    min="0"
                    max="1"
                    step="0.1"
                    value={track.volume}
                    onChange={(e) =>
                      updateTrackVolume(track.id, parseFloat(e.target.value))
                    }
                    className="flex-1"
                  />
                </div>
                <div className="flex-1 flex items-center gap-4">
                  <span className="text-white">BPM</span>
                  <input
                    type="range"
                    min="0.5"
                    max="2"
                    step="0.1"
                    value={track.playbackRate}
                    onChange={(e) =>
                      updateTrackBPM(track.id, parseFloat(e.target.value))
                    }
                    className="flex-1"
                  />
                </div>
                <button
                  onClick={() => deleteTrack(track.id)}
                  className="text-red-400 hover:text-red-300 transition-colors"
                >
                  <Trash2 size={20} />
                </button>
              </div>
            ))}

            {tracks.length === 0 && (
              <div className="text-center text-gray-400 py-8">
                <Plus size={40} className="mx-auto mb-2" />
                <p>Upload MP3 files to add tracks</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;
