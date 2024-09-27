(ns user
  (:require [clojure.java.io :as io])
  (:import [java.io FileInputStream StringWriter]
           java.net.Socket))

(def hsize 4) ;; header size
(def dsize 1024) ;; data size, 4*256
(def psize (+ hsize dsize)) ;; packet size

(defn deploy [server-ip server-port bin-file]
  (let [f (io/file bin-file)
        ba (byte-array psize)]
    (when-not (.exists f)
      (throw (ex-info "File not found!" {})))
    (println "Deploying file length:" (.length f))
    (flush)
    (Thread/sleep 1000)
    (with-open [sock (Socket. server-ip server-port)
                sos (.getOutputStream sock)
                sis (.getInputStream sock)
                fis (FileInputStream. f)]
      (println "connected..")
      (flush)
      ;; init flash
      (aset-byte ba 0 1)
      (.write sos ba 0 1)
      (.flush sos)
      (Thread/sleep 2)
      (.read sis)
      (println "sent init command")
      (flush)
      ;; flash data
      (aset-byte ba 0 2)
      (loop [offset 0] ;; offset by count of dsize blocks
        (aset-byte ba 1 (unchecked-byte (quot offset 0x100)))
        (aset-byte ba 2 (unchecked-byte (mod offset 0x100)))
        (when (pos? (.read fis ba hsize dsize))
          (.write sos ba 0 psize)
          (.flush sos)
          (Thread/sleep 2)
          (.read sis)
          (println "sent data packet" offset)
          (flush)
          (recur (inc offset))))
      (println "sent all data packets")
      (flush)
      ;; end flash
      (aset-byte ba 0 3)
      (.write sos ba 0 1)
      (.flush sos)
      (Thread/sleep 2)
      (.read sis)
      (println "Finished deploy")
      (flush))))

(comment ;; deploy

  "NOTE:
Initial deployment:
   1. copy the pico-fota-bootloader.uf2 to picow usb drive
   2. copy the kbd_(ap|left|right).uf2 to picow usb drive

Thereafter, this wifi download method can be used to update the picow."

  (deploy "192.168.4.1" 82 "/home/dipu/my/pico/kbd/picow/build/kbd/kbd_ap.bin")

  (do

    (deploy "192.168.4.2" 82 "/home/dipu/my/pico/kbd/picow/build/kbd/kbd_left.bin")

    (deploy "192.168.4.3" 82 "/home/dipu/my/pico/kbd/picow/build/kbd/kbd_right.bin")

    )


  )

(comment

  (deploy "192.168.4.1" 80 "/home/dipu/my/pico/kbd/picow/build/hello_wifi/picow_ap.bin")

  (deploy "192.168.4.3" 82 "/home/dipu/my/pico/kbd/picow/build/hello_wifi/picow_client.bin")



  )

(comment

  (defn send-request
    [host port]
    (with-open [sock (Socket. host port)
                writer (io/writer sock)
                reader (io/reader sock)
                response (StringWriter.)]
      (let [t0 (System/nanoTime)]
        (.append writer "Hello")
        (.write writer 0)
        (.flush writer)
        (loop []
          (let [c (.read reader)]
            (println "received:" c)
            (.write response c)
            (if (zero? c)
              (println (str response) "time:" (quot (- (System/nanoTime) t0) 1000))
              (recur)))))))

  (defn time-request
    [host port n]
    (with-open [sock (Socket. host port)
                writer (io/writer sock)
                reader (io/reader sock)]
      (let [a (char-array n \X)
            t0 (System/currentTimeMillis)
            _ (.write writer a 0 n)
            _ (.flush writer)
            nr (.read reader a 0 n)
            t1 (System/currentTimeMillis)]
        (println "received:" nr "time:" (- t1 t0)))))

  (comment

    (send-request "192.168.4.1" 80)

    (time-request "192.168.4.1" 80 1000)

    )

  )
