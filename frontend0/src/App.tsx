import React, { useState, useRef, useEffect } from 'react';
import { Mic, Square, Play, Plus, Trash2, Volume2 } from 'lucide-react';

interface Track {
  id: string;
  buffer: AudioBuffer | null;
  volume: number;
}

function App() {
  const [isRecording, setIsRecording] = useState(false);
  const [tracks, setTracks] = useState<Track[]>([]);
  const [isPlaying, setIsPlaying] = useState(false);
  
  const audioContext = useRef<AudioContext | null>(null);
  const mediaRecorder = useRef<MediaRecorder | null>(null);
  const recordedChunks = useRef<Blob[]>([]);
  const startTime = useRef<number>(0);
  
  // Initialize audio context
  useEffect(() => {
    audioContext.current = new AudioContext();
    return () => {
      audioContext.current?.close();
    };
  }, []);

  // Request microphone access and start recording
  const startRecording = async () => {
    try {
      const stream = await navigator.mediaDevices.getUserMedia({ audio: true });
      mediaRecorder.current = new MediaRecorder(stream);
      recordedChunks.current = [];

      mediaRecorder.current.ondataavailable = (e) => {
        if (e.data.size > 0) {
          recordedChunks.current.push(e.data);
        }
      };

      mediaRecorder.current.onstop = async () => {
        const blob = new Blob(recordedChunks.current, { type: 'audio/webm' });
        const arrayBuffer = await blob.arrayBuffer();
        const audioBuffer = await audioContext.current!.decodeAudioData(arrayBuffer);
        
        setTracks(prev => [...prev, {
          id: Date.now().toString(),
          buffer: audioBuffer,
          volume: 1
        }]);
      };

      mediaRecorder.current.start();
      setIsRecording(true);
      startTime.current = audioContext.current!.currentTime;
    } catch (err) {
      console.error('Error accessing microphone:', err);
    }
  };

  const stopRecording = () => {
    if (mediaRecorder.current && isRecording) {
      mediaRecorder.current.stop();
      mediaRecorder.current.stream.getTracks().forEach(track => track.stop());
      setIsRecording(false);
    }
  };

  const playTracks = () => {
    if (!audioContext.current || tracks.length === 0) return;
    
    setIsPlaying(true);
    const startTime = audioContext.current.currentTime;
    
    tracks.forEach(track => {
      if (!track.buffer) return;
      
      const source = audioContext.current!.createBufferSource();
      const gainNode = audioContext.current!.createGain();
      
      source.buffer = track.buffer;
      source.loop = true;
      
      gainNode.gain.value = track.volume;
      
      source.connect(gainNode);
      gainNode.connect(audioContext.current!.destination);
      
      source.start(startTime);
    });
  };

  const stopPlayback = () => {
    if (!audioContext.current) return;
    audioContext.current.close();
    audioContext.current = new AudioContext();
    setIsPlaying(false);
  };

  const deleteTrack = (id: string) => {
    setTracks(prev => prev.filter(track => track.id !== id));
  };

  const updateTrackVolume = (id: string, volume: number) => {
    setTracks(prev => prev.map(track => 
      track.id === id ? { ...track, volume } : track
    ));
  };

  return (
    <div className="min-h-screen bg-gradient-to-br from-purple-900 to-indigo-900 p-8">
      <div className="max-w-4xl mx-auto">
        <h1 className="text-4xl font-bold text-white mb-8 text-center">React Audio Looper</h1>
        
        <div className="bg-white/10 backdrop-blur-lg rounded-lg p-6 mb-8">
          <div className="flex justify-center gap-4 mb-8">
            {!isRecording ? (
              <button
                onClick={startRecording}
                className="flex items-center gap-2 bg-red-500 hover:bg-red-600 text-white px-6 py-3 rounded-full transition-colors"
              >
                <Mic size={20} />
                Start Recording
              </button>
            ) : (
              <button
                onClick={stopRecording}
                className="flex items-center gap-2 bg-gray-700 hover:bg-gray-800 text-white px-6 py-3 rounded-full transition-colors"
              >
                <Square size={20} />
                Stop Recording
              </button>
            )}
            
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
                <span className="text-white font-medium">Track {index + 1}</span>
                <div className="flex-1 flex items-center gap-4">
                  <Volume2 size={20} className="text-white" />
                  <input
                    type="range"
                    min="0"
                    max="1"
                    step="0.1"
                    value={track.volume}
                    onChange={(e) => updateTrackVolume(track.id, parseFloat(e.target.value))}
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
                <p>Start recording to add tracks</p>
              </div>
            )}
          </div>
        </div>
      </div>
    </div>
  );
}

export default App;