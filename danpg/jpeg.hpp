//
//  jpeg.hpp
//  danpg
//
//  Created by Daniel Burke on 2/5/2023.
//

#ifndef jpeg_hpp
#define jpeg_hpp

#include <istream>

class Jpeg {
private:
    std::vector<uint8_t> _identifier;
    uint16_t _version;
    uint8_t _units;
    
    typedef std::array<uint8_t, 64> QuantisationTable;
    std::array<QuantisationTable, 4> _quantTables;
    
    struct HuffmanTable {
        std::array<uint8_t, 16> bits;
        std::vector<uint8_t> huffsize;
        std::unordered_map<size_t, uint16_t> huffcode;
    };
    std::array<HuffmanTable, 4> _huffmanTablesAC;
    std::array<HuffmanTable, 4> _huffmanTablesDC;
    
    uint8_t _frameSamplePrecision;
    uint16_t _y; //number of lines
    uint16_t _x; //number of samples per line
    uint8_t _nf; //number of image components in frame
    
    struct ImageComponent {
        uint8_t _c; //component identifier
        uint8_t _h; //horizontal sampling factor
        uint8_t _v; //vertical sampling factor
        uint8_t _tq; //quantization table destination selector
    };
    std::vector<ImageComponent> _imageComponents;
    
    struct ImageComponentInScan {
        uint8_t _cs; //scan component selector
        uint8_t _td; //dc entropy coding table destination selector
        uint8_t _ta; //ac entropy coding table destination selector
    };
    std::vector<ImageComponentInScan> _imageComponentsInScan;
    
    size_t _numberOfMCU;
    
public:
    Jpeg(std::istream& is);
    
    void appZeroData(std::vector<uint8_t>& data);
    void quantisationTable(std::vector<uint8_t>& data);
    void huffmanTable(std::vector<uint8_t>& data);

    void sofBaselineDCT(std::vector<uint8_t>& data);
    void startOfScan(std::vector<uint8_t>& data);
    void restartInterval(std::vector<uint8_t>& data);
    
    
};

#endif /* jpeg_hpp */
