      if (
        countPendingFiles() > 0
        && WiFi.status() == WL_CONNECTED
      ) {
        retryOnePending();
      }
    }

    // Riconnessione automatica se il WiFi cade.
    static uint32_t lastReconnectCheck = 0;

    if (
      millis() - lastReconnectCheck
      > 10000
    ) {
      lastReconnectCheck =
        millis();

      if (
        WiFi.status()
        != WL_CONNECTED
      ) {
        Serial.println(
          "WiFi perso: tentativo riconnessione"
        );

        WiFi.reconnect();
      }
    }
  }

  delay(1);
}
