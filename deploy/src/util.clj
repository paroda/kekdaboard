(ns util
  (:require [clojure.string :as str]
            [clojure.java.io :as jio]
            [jsonista.core :as json]))

(def eabi-include #{"-I/usr/arm-none-eabi/include"})

(defn inject-eabi [{:strs [arguments] :as m}]
  (let [args (remove eabi-include arguments)
        a1 (take-while #(not (str/starts-with? % "-I")) args)
        args
        (vec (concat a1
                     eabi-include
                     (drop (count a1) args)))]
    (assoc m "arguments" args)))

(comment

  (with-open [is (jio/input-stream "../picow/build/compile_commands.json")]
    (def j (json/read-value is)))

  (first j)

  (def j2 (->> j
               (filter (fn [{:strs [file] :as m}]
                         ;; (str/starts-with? file "/home/dipu/my/pico/kbd/picow/hello_bt")
                         (str/starts-with? file "/home/dipu/my/pico/kbd/picow/kbd")
                         ))
               (map inject-eabi)
               ))

  (count j2)

  (with-open [os (jio/output-stream "../picow/build/compile_commands.json")]
    (json/write-value os j2))

  )
