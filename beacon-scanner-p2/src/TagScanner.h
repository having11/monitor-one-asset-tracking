#pragma once

#include <SPI.h>
#include <MFRC522.h>

#include <memory>
#include <optional>

class TagScanner {
  public:
    TagScanner(SPIClass* spi, uint8_t rstPin, uint8_t csPin);
    void init();
    bool update();
    /**
     * @brief Read the card's ID
     * 
     * @return std::optional<int16_t> non-null if card is present
     */
    std::optional<int16_t> getCardId();
    /**
     * @brief Writes a new ID to the card
     * 
     * @param id 
     * @return std::optional<bool> non-null if card is present
     */
    std::optional<bool> writeCardId(int16_t id);

  private:
    uint8_t _rstPin;
    uint8_t _csPin;
    std::unique_ptr<MFRC522> _reader;
};