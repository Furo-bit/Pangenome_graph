#include "utils.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <random>
#include <graphviz/gvc.h>
#include <graphviz/cgraph.h> 
#include <set>

using namespace std;


// Funzione per creare un Block
Block create_block(const string& id, const string& label, 
                   const vector<int>& sequence_ids, int begin_column, int end_column) {
    Block block;
    block.id = id;
    block.label = label;
    block.sequence_ids = sequence_ids;
    block.begin_column = begin_column;
    block.end_column = end_column;
    
    return block;
}

// Funzione per stampare i dettagli di un Block
void print_block(const Block& block) {
    cout << "ID: " << block.id << ", "
         << "Label: " << block.label << ", "
         << "Sequence IDs: ";
    for (int id : block.sequence_ids) {
        cout << id << " ";
    }
    cout << ", Begin Column: " << block.begin_column
         << ", End Column: " << block.end_column
         << endl;
}

// Funzione per leggere un file FASTA e restituire le sequenze
unordered_map<string, vector<char>> read_fasta(const string& filename) {
    unordered_map<string, vector<char>> sequences;
    ifstream file(filename);

    if (!file.is_open()) {
        cerr << "Errore nell'apertura del file: " << filename << endl;
        return sequences;
    }

    string line;
    string current_id;

    while (getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        if (line[0] == '>') {
            current_id = line.substr(1);
            sequences[current_id] = vector<char>();
        } else {
            sequences[current_id].insert(sequences[current_id].end(), line.begin(), line.end());
        }
    }

    file.close();
    return sequences;
}

// Funzione per convertire le sequenze in una matrice
vector<vector<char>> sequences_to_matrix(const unordered_map<string, vector<char>>& sequences) {
    vector<vector<char>> matrix;
    
    if (sequences.empty()) {
        return matrix;
    }
    
    size_t length = sequences.begin()->second.size();

    for (const auto& pair : sequences) {
        /*
        if (pair.second.size() != length) {
            cerr << "Le sequenze non hanno la stessa lunghezza!" << endl;
            return {};
        }
        */
        matrix.push_back(pair.second);
    }
    
    return matrix;
}

// Funzione per costruire blocchi da una matrice di sequenze
pair<unordered_map<string, Block>, vector<vector<string>>> build_blocks_from_sequence_matrix(
    const vector<vector<char>>& sequence_matrix) {
    
    unordered_map<string, Block> block_dict;
    size_t num_seq = sequence_matrix.size();
    size_t num_bases = sequence_matrix.empty() ? 0 : sequence_matrix[0].size() - 1;
    vector<vector<string>> block_id_matrix(num_seq, vector<string>(num_bases, ""));
    
    for (size_t sequence_index = 0; sequence_index < num_seq; ++sequence_index) {
        for (size_t base_index = 0; base_index < num_bases; ++base_index) {
            string block_id = "B" + to_string(sequence_index) + "." + to_string(base_index);
            Block block = create_block(
                block_id,
                string(1, sequence_matrix[sequence_index][base_index]),
                vector<int>{static_cast<int>(sequence_index)},
                static_cast<int>(base_index),
                static_cast<int>(base_index)
            );
            block_dict[block_id] = block;
            block_id_matrix[sequence_index][base_index] = block_id;
        }
    }
    
    return {block_dict, block_id_matrix};
}

// Funzione per ordinare i blocchi in base al sequence_id
vector<Block> sort_blocks_by_sequence_id(const unordered_map<string, Block>& block_dict) {
    vector<Block> blocks;
    
    for (const auto& pair : block_dict) {
        blocks.push_back(pair.second);
    }
    
    sort(blocks.begin(), blocks.end(), [](const Block& a, const Block& b) {
        return a.sequence_ids[0] < b.sequence_ids[0];
    });
    
    return blocks;
}

void printEdgeInfo(Agedge_t* e) {
    if (e == nullptr) {
        cout << "Errore: Arco non trovato" << endl;
        return;
    }

    // Ottenere i nodi sorgente e destinazione dell'arco
    Agnode_t* source = agtail(e);  // Nodo sorgente
    Agnode_t* target = aghead(e);  // Nodo destinazione

    // Ottenere l'etichetta dell'arco (se esiste)
    char* edge_label = agget(e, (char*)"label");
    if (edge_label == nullptr || edge_label[0] == '\0') {
        edge_label = (char*)"<no label>";
    }


    // Stampa le informazioni
    cout << "Informazioni sull'arco:" << endl;
    cout << "  Nodo sorgente: " << agnameof(source) << endl;
    cout << "  Nodo destinazione: " << agnameof(target) << endl;
    cout << "  Etichetta: " << edge_label << endl;
}

// Funzione per unire due blocchi
Block merge_two_blocks(const Block& block_1, const Block& block_2, const string& how) {
    const vector<string> valid_values = {"column_union", "row_union"};
    if (find(valid_values.begin(), valid_values.end(), how) == valid_values.end()) {
        throw invalid_argument("Invalid value for param: " + how + ", the accepted values are column_union and row_union");
    }

    Block new_block;

    if (how == "column_union") {
        new_block.id = block_1.id;
        new_block.label = block_1.label + block_2.label;
        new_block.sequence_ids = block_1.sequence_ids; // Assumendo che sequence_ids siano rappresentati come un vettore di int
        new_block.begin_column = block_1.begin_column;
        new_block.end_column = block_2.end_column;
    } else if (how == "row_union") {
        new_block.id = block_1.id;
        new_block.label = block_1.label;
        new_block.sequence_ids = block_1.sequence_ids; // Unione dei sequence_ids
        new_block.sequence_ids.insert(new_block.sequence_ids.end(), block_2.sequence_ids.begin(), block_2.sequence_ids.end());
        new_block.begin_column = block_1.begin_column;
        new_block.end_column = block_1.end_column;
    }

    return new_block;
}

// Funzione aggiornata per eliminare i blocchi id1 e id2 e aggiungere il nuovo blocco
unordered_map<string, Block> update_block_dict_with_same_id(
    unordered_map<string, Block>& block_dict, const Block& new_block, const string& id1, const string& id2) {
    
    // Elimina i blocchi con ID id1 e id2
    block_dict.erase(id1);
    block_dict.erase(id2);
    
    // Aggiungi il nuovo blocco al dizionario
    block_dict[new_block.id] = new_block;
    
    return block_dict;
}

void update_block_submatrix_with_same_id(vector<vector<string>>& block_id_matrix, 
                                         const string& new_id, 
                                         const vector<int>& sequences, 
                                         int first_column, 
                                         int last_column) {
    // Aggiorna gli ID dei blocchi nelle righe specificate e nelle colonne comprese tra first_column e last_column
    for (int sequence : sequences) {
        for (int column = first_column; column <= last_column; ++column) {
            block_id_matrix[sequence][column] = new_id;
        }
    }
}

float acceptance_probability(float delta, float temperature, float beta) {
    return exp(-beta * delta / temperature);
}

// Genera una lista di numeri casuali
vector<int> generate_random_numbers(int seed, int start, int end, int count) {
    vector<int> random_numbers;
    mt19937 generator(seed);  // Generatore di numeri casuali con seme
    uniform_int_distribution<int> distribution(start, end);  // Distribuzione uniforme tra start e end

    for (int i = 0; i < count; ++i) {
        random_numbers.push_back(distribution(generator));
    }

    return random_numbers;
}

// Genera un singolo numero casuale, se rifatto con gli stessi parametri dà lo stesso risultato
int generate_random_number(int seed, int start, int end) {
    mt19937 generator(seed);  // Generatore di numeri casuali con seme
    uniform_int_distribution<int> distribution(start, end);  // Distribuzione uniforme tra start e end
    return distribution(generator);
}

// Funzione per dividere un blocco per riga
tuple<Block, Block> split_block_by_row(const Block& block, int split_number, int split_point) {
    const vector<int>& sequence_ids = block.sequence_ids;

    if (sequence_ids.size() <= 1) {
        throw invalid_argument("Block must contain at least two sequence IDs to be split.");
    }

    // Creazione dei nuovi ID dei blocchi
    string id1, id2;
    size_t underscore_index = block.id.find('_');

    if (underscore_index != string::npos) {
        id1 = block.id.substr(0, underscore_index + 1) + to_string(split_number);
        id2 = block.id.substr(0, underscore_index + 1) + to_string(split_number + 1);
    } else {
        id1 = block.id + '_' + to_string(split_number);
        id2 = block.id + '_' + to_string(split_number + 1);
    }

    // Creazione dei due nuovi blocchi
    Block part1 = {
        id1,
        block.label,
        vector<int>(sequence_ids.begin(), sequence_ids.begin() + split_point),
        block.begin_column,
        block.end_column
    };

    Block part2 = {
        id2,
        block.label,
        vector<int>(sequence_ids.begin() + split_point, sequence_ids.end()),
        block.begin_column,
        block.end_column
    };
    return make_tuple(part1, part2);
}


// Funzione per dividere un blocco per colonna
tuple<Block, Block> split_block_by_column(const Block& block, int split_number, int split_point) {
    const string& label = block.label;
    int begin_column = block.begin_column;
    int end_column = block.end_column;

    if (end_column - begin_column <= 0) {
        throw invalid_argument("Block must span at least two columns to be split.");
    }


    // Creazione dei nuovi ID dei blocchi
    string id1, id2;
    size_t underscore_index = block.id.find('_');

    if (underscore_index != string::npos) {
        id1 = block.id.substr(0, underscore_index + 1) + to_string(split_number);
        id2 = block.id.substr(0, underscore_index + 1) + to_string(split_number + 1);
    } else {
        id1 = block.id + '_' + to_string(split_number);
        id2 = block.id + '_' + to_string(split_number + 1);
    }

    // Creazione dei due nuovi blocchi
    Block part1 = {
        id1,
        label.substr(0, split_point - begin_column),
        block.sequence_ids,
        begin_column,
        split_point - 1
    };

    Block part2 = {
        id2,
        label.substr(split_point - begin_column),
        block.sequence_ids,
        split_point,
        end_column
    };
    return make_tuple(part1, part2);
}


unordered_map<string, Block> greedy_row_merge(unordered_map<string, Block>& block_dict, vector<vector<string>>& block_id_matrix) {
    size_t num_seq = block_id_matrix.size();
    size_t num_bases = block_id_matrix[0].size();

    for (size_t i = 0; i < num_seq; ++i) {
        for (size_t j = 0; j < num_bases; ++j) {
            for (size_t z = 0; z < num_seq; ++z) {
                string id_1 = block_id_matrix[i][j];
                string id_2 = block_id_matrix[z][j];

                if (id_1 != id_2 && block_dict[id_1].label == block_dict[id_2].label) {
                    Block& block_1 = block_dict[id_1];
                    Block& block_2 = block_dict[id_2];
                    
                    Block new_block;
                    if (z < i) {
                        new_block = merge_two_blocks(block_2, block_1, "row_union");
                        block_dict = update_block_dict_with_same_id(block_dict, new_block, block_2.id, block_1.id);
                    } else {
                        new_block = merge_two_blocks(block_1, block_2, "row_union");
                        block_dict = update_block_dict_with_same_id(block_dict, new_block, block_1.id, block_2.id);
                    }

                    for (int sequence : new_block.sequence_ids) {
                        for (int column = new_block.begin_column; column <= new_block.end_column; ++column) {
                            block_id_matrix[sequence][column] = new_block.id;
                        }
                    }
                }
            }
        }
    }

    return block_dict;
}

int of_min_label_length_threshold(int threshold, int penalization, int label_length) {
    if (label_length < threshold) {
        return penalization * (threshold - label_length);
    } else {
        return 1;
    }
}

int of_pangeblocks(int threshold, int penalization, int label_length) {
    if (label_length < threshold) {
        return penalization;
    } else {
        return 1;
    }
}

bool check_number_in_string(int number, const string& str) {
    stringstream ss(str);
    string item;
    
    while (getline(ss, item, ',')) {
        if (to_string(number) == item) {
            return true;
        }
    }
    
    return false;
}

bool check_id_matrix_consistency(const vector<vector<string>>& block_id_matrix, 
                                 const unordered_map<string, Block>& block_dict, 
                                 const vector<vector<char>>& sequence_matrix) {
    size_t rows = block_id_matrix.size();
    size_t cols = block_id_matrix[0].size();

    for (size_t row = 0; row < rows; ++row) {
        string sequence = "";
        string previous_id = "none";

        for (size_t col = 0; col < cols; ++col) {
            const Block& current_block = block_dict.at(block_id_matrix[row][col]);

            if (current_block.id != previous_id) {
                sequence += current_block.label;
                previous_id = current_block.id;
            }
        }

        // Verifica se la sequenza generata corrisponde alla sequenza nella matrice
        string original_sequence(sequence_matrix[row].begin(), sequence_matrix[row].end());
        if (original_sequence != sequence) {
            cout << "Inconsistency found in row: " << row << endl;
            cout << "Expected sequence: " << original_sequence << endl;
            cout << "Generated sequence: " << sequence << endl;
            return false;
        }
    }
    return true;
}



void graph_to_gfa(Agraph_t* g, const string& filename) {
    ofstream outfile(filename);
    if (!outfile.is_open()) {
        cerr << "Errore nell'apertura del file GFA" << endl;
        return;
    }

    // Scrive l'intestazione del file GFA
    outfile << "H\tVN:Z:1.0\n";

    // Itera attraverso i nodi del grafo
    for (Agnode_t* node = agfstnode(g); node; node = agnxtnode(g, node)) {
        // Ottiene l'etichetta del nodo (se non esiste, usa '*')
        char* label = agget(node, const_cast<char*>("label"));
        if (label == nullptr) {
            label = const_cast<char*>("*");
        }
        // Scrive la linea del nodo nel formato GFA
        outfile << "S\t" << agnameof(node) << "\t*\tLN:i:" << label << "\n";
    }

    // Itera attraverso gli archi del grafo
    for (Agnode_t* node = agfstnode(g); node; node = agnxtnode(g, node)) {
        for (Agedge_t* edge = agfstout(g, node); edge; edge = agnxtout(g, edge)) {
            // Ottiene l'etichetta dell'arco (se non esiste, usa '*')
            char* edge_label = agget(edge, const_cast<char*>("label"));
            if (edge_label == nullptr || strlen(edge_label) == 0) {
                edge_label = const_cast<char*>("*");
            }
            // Scrive la linea dell'arco nel formato GFA
            outfile << "L\t" << agnameof(agtail(edge)) << "\t+\t"
                    << agnameof(aghead(edge)) << "\t+\t" << edge_label << "\n";
        }
    }

    outfile.close();
}


// Funzione che rimuove i nodi con label formate solo da caratteri "-"
void remove_indel_nodes(Agraph_t* g) {
    vector<Agnode_t*> nodes_to_remove;
    
    // Itera sui nodi del grafo
    for (Agnode_t* n = agfstnode(g); n; n = agnxtnode(g, n)) {
        char* label = agget(n, const_cast<char*>("label"));
        
        if (label) {
            string label_str(label);

            // Se la label è composta solo da "-", contrassegna il nodo per la rimozione
            if (all_of(label_str.begin(), label_str.end(), [](char c) { return c == '-'; })) {
                nodes_to_remove.push_back(n);
            }
        }
    }

    // Rimuove i nodi contrassegnati
    for (Agnode_t* node : nodes_to_remove) {
        agdelnode(g, node);
    }
}

// Funzione per leggere il file GFA e correggere gli archi
void processGFA(const string& input_file, const string& output_file) {
    ifstream infile(input_file);
    ofstream outfile(output_file);

    if (!infile.is_open()) {
        cerr << "Errore nell'aprire il file di input!" << endl;
        return;
    }

    if (!outfile.is_open()) {
        cerr << "Errore nell'aprire il file di output!" << endl;
        return;
    }

    unordered_map<string, set<string>> edge_map;  // Mappa che tiene traccia degli archi e delle loro label
    vector<string> lines;  // Memorizza tutte le righe non di archi

    string line;
    while (getline(infile, line)) {
        if (line[0] != 'L') {  // Righe non di archi
            lines.push_back(line);
        } else {
            // Linea dell'arco, separiamo i campi
            istringstream iss(line);
            string type, source, source_dir, target, target_dir, label;
            getline(iss, type, '\t');  // 'L'
            getline(iss, source, '\t');
            getline(iss, source_dir, '\t');
            getline(iss, target, '\t');
            getline(iss, target_dir, '\t');
            getline(iss, label, '\t');

            // Creiamo una chiave unica per l'arco (source-target)
            string edge_key = source + "->" + target;

            // Aggiungi la label alla mappa
            edge_map[edge_key].insert(label);
        }
    }

    // Scriviamo le righe non di archi nel file di output
    for (const string& l : lines) {
        outfile << l << endl;
    }

    // Scriviamo gli archi unificati nel file di output
    for (const auto& pair : edge_map) {
        string edge_key = pair.first;
        const set<string>& labels = pair.second;

        // Separiamo source e target
        size_t arrow_pos = edge_key.find("->");
        string source = edge_key.substr(0, arrow_pos);
        string target = edge_key.substr(arrow_pos + 2);

        // Concatenazione delle label
        string concatenated_labels;
        for (const auto& label : labels) {
            if (!concatenated_labels.empty()) {
                concatenated_labels += ",";
            }
            concatenated_labels += label;
        }

        // Scrivi l'arco nel file di output con le label concatenate
        outfile << "L\t" << source << "\t+\t" << target << "\t+\t" << concatenated_labels << endl;
    }

    infile.close();
    outfile.close();
}



// Funzione per costruire il grafo
Agraph_t* build_graph(const unordered_map<string, Block>& block_dict, const vector<vector<string>>& block_id_matrix) {

    // Crea un contesto Graphviz
    GVC_t *gvc = gvContext();

    // Crea un grafo diretto
    Agraph_t *g = agopen(const_cast<char*>("G"), Agdirected, NULL);

    // Aggiungi i nodi
    // source e sink
    string label_name = "label";
    string source_name = "Source";
    Agnode_t *source = agnode(g, const_cast<char*>(source_name.c_str()), true);
    agset(source, const_cast<char*>(label_name.c_str()), const_cast<char*>(source_name.c_str()));
    string sink_name = "Sink";
    Agnode_t *sink = agnode(g, const_cast<char*>(sink_name.c_str()), true);
    agset(sink, const_cast<char*>(label_name.c_str()), const_cast<char*>(sink_name.c_str()));

    // nodi dal dizionario
    for (const auto& pair : block_dict) {
        //ogni nodo deve avere ID che sarà il nome, label che sarà la label ma tolta degli indel
        const string& block_id = pair.first;
        const Block& block = pair.second;
        Agnode_t* node = agnode(g, const_cast<char*>(block_id.c_str()), TRUE);
        string label = remove_chars(block.label, "-");
        agset(node, const_cast<char*>("label"), const_cast<char*>(label.c_str()));
    }

    

    // aggiunta archi
    // nuova soluzione
    
    // Iterare su tutti gli elementi della mappa e inserire le chiavi nel vettore
    vector<string> keys;
    for (const auto& pair : block_dict) {
        keys.push_back(pair.first);  
    }
    
    // Ottenere la dimensione della mappa
    size_t n = block_dict.size();

    // Creare una matrice n x n di stringhe vuote
    vector<vector<string>> label_matrix(n, vector<string>(n, "")); // inizializza con stringhe vuote

    //creo e salvo nella matrice delle label le etichette degli archi
    size_t rows = block_id_matrix.size();
    size_t cols = block_id_matrix[0].size();

    agattr(g, AGEDGE, const_cast<char*>("label"), const_cast<char*>(""));

    
    // archi normali
    for (size_t row = 0; row < rows; ++row) {
        for (size_t col = 0; col < cols - 1; ++col) {
            string block_1_id = block_id_matrix[row][col];
            const Block& block_1 = block_dict.at(block_1_id);
            if (!all_of(block_1.label.begin(), block_1.label.end(), [](char c) { return c == '-'; })) {
                for (size_t col2 = col + 1; col2 < cols; ++col2) {
                    string block_2_id = block_id_matrix[row][col2];
                    const Block& block_2 = block_dict.at(block_2_id);
                    if (!all_of(block_2.label.begin(), block_2.label.end(), [](char c) { return c == '-'; }) && block_1_id != block_2_id) {
                        
                        //aggiorno il valore della label
                        for (size_t i = 0; i < n; ++i) {
                            if (block_1_id == keys[i]){
                                for (size_t j = 0; j < n; ++j) {
                                    if (block_2_id == keys[j]){
                                        string label = label_matrix[i][j];
                                        if (label == ""){
                                            label = to_string(row);
                                            label_matrix[i][j] = label;
                                        }
                                        else{
                                            if (!check_number_in_string(row, label)){
                                                label = label + "," + to_string(row);
                                                label_matrix[i][j] = label;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        

                        break;
                    }
                }
            }
        }
    }

    // creo gli archi normali
    for (size_t i = 0; i < n; ++i) {
        for (size_t j = 0; j < n; ++j) {
           if (label_matrix[i][j] != ""){
                string block_1_id = keys[i];
                string block_2_id = keys[j];
                
                // Trova il nodo per block_1_id
                Agnode_t* node_1 = agnode(g, const_cast<char*>(block_1_id.c_str()), false);
                
                // Trova il nodo per block_2_id
                Agnode_t* node_2 = agnode(g, const_cast<char*>(block_2_id.c_str()), false);
                
                Agedge_t* e = agedge(g, node_1, node_2, nullptr, true);
                agset(e, (char*)"label", strdup(label_matrix[i][j].c_str()));
           }
        }      
    }
    
    // archi source
    vector<string> source_labels(n, "");

    for (size_t row = 0; row < rows; ++row) {
        for (size_t col = 0; col < cols - 1; ++col) {
            string block_id = block_id_matrix[row][col];
            const Block& block = block_dict.at(block_id);
            if (!all_of(block.label.begin(), block.label.end(), [](char c) { return c == '-'; })) {
                 
            //aggiorno il valore della label
                for (size_t i = 0; i < n; ++i) {
                    if (block_id == keys[i]){
                       
                        string label = source_labels[i];
                        if (label == ""){
                            label = to_string(row);
                            source_labels[i] = label;
                        }
                        else{
                            if (!check_number_in_string(row, label)){
                                label = label + "," + to_string(row);
                                source_labels[i] = label;
                            }
                        }   
                    }
                }
                break;
            }
        }
    }
    
    // creo gli archi source
    for (size_t i = 0; i < n; ++i) {   
        if (source_labels[i] != ""){
            string block_1_id = keys[i];

            // Trova il nodo per block_1_id
            Agnode_t* node = agnode(g, const_cast<char*>(block_1_id.c_str()), false);
            Agedge_t* e = agedge(g, source, node, nullptr, true);
            agset(e, (char*)"label", strdup(source_labels[i].c_str()));
        }     
    }


    // archi sink
    vector<string> sink_labels(n, "");

    for (size_t row = 0; row < rows; ++row) {
        for (size_t col = cols - 1; col >= 0; --col) {
            string block_id = block_id_matrix[row][col];
            const Block& block = block_dict.at(block_id);
            if (!all_of(block.label.begin(), block.label.end(), [](char c) { return c == '-'; })) {
                 
            //aggiorno il valore della label
                for (size_t i = 0; i < n; ++i) {
                    if (block_id == keys[i]){
                       
                        string label = sink_labels[i];
                        if (label == ""){
                            label = to_string(row);
                            sink_labels[i] = label;
                        }
                        else{
                            if (!check_number_in_string(row, label)){
                                label = label + "," + to_string(row);
                                sink_labels[i] = label;
                            }
                        }   
                    }
                }
                break;
            }
        }
    }
    
    // creo gli archi sink
    for (size_t i = 0; i < n; ++i) {   
        if (sink_labels[i] != ""){
            string block_1_id = keys[i];

            // Trova il nodo per block_1_id
            Agnode_t* node = agnode(g, const_cast<char*>(block_1_id.c_str()), false);
            Agedge_t* e = agedge(g, node, sink, nullptr, true);
            agset(e, (char*)"label", strdup(sink_labels[i].c_str()));
        }     
    }
    

    remove_indel_nodes(g);
    //print_graph(g);

    // Esportazione del grafo in formato GFA
    graph_to_gfa(g, "graph.gfa");
    //processGFA("graph.gfa", "clean_graph.gfa"); 

    // Genera layout e renderizza il grafo
    //gvLayout(gvc, g, "dot");
    //gvRenderFilename(gvc, g, "png", "graph.png");
    gvFreeLayout(gvc, g);

    return g;
}


// Funzione di utilità per convertire string in char*
char* toCharPointer(const string& str) {
    return const_cast<char*>(str.c_str());
}


// Funzione di test per visualizzare i nodi e le loro etichette e gli archi
void print_graph(Agraph_t* g) {
    // Stampa i nodi e le loro etichette
    for (Agnode_t* n = agfstnode(g); n; n = agnxtnode(g, n)) {
        char* label = agget(n, const_cast<char*>("label"));
        cout << "Node: " << agnameof(n) << " Label: " << (label ? label : "No Label") << endl;

        // Itera sugli archi uscenti dal nodo
        for (Agedge_t* e = agfstout(g, n); e; e = agnxtout(g, e)) {
            cout << "Edge: " << agnameof(agtail(e)) << " -> " << agnameof(aghead(e)) << endl;
        }
    }
}


// Funzione per rimuovere i nodi con etichette costituite solo da "-" e aggiornare i collegamenti
void remove_nodes_with_dash_labels(Agraph_t *g) {
    vector<Agnode_t*> nodes_to_remove;

    // Trova i nodi con etichette costituite solo da "-"
    for (Agnode_t* node = agfstnode(g); node; node = agnxtnode(g, node)) {
        const char* label = agget(node, const_cast<char*>("label"));
        string label_str = (label ? label : "");
        
        if (label_str.find_first_not_of('-') == string::npos) {
            nodes_to_remove.push_back(node);
        }
    }

    // Trova i collegamenti da rimuovere e aggiorna i collegamenti
    for (Agnode_t* node : nodes_to_remove) {
        vector<Agnode_t*> predecessors;
        vector<Agnode_t*> successors;

        // Trova i predecessori e i successori
        for (Agedge_t* e = agfstedge(g, node); e; e = agnxtedge(g, e, node)) {
            Agnode_t* pred = aghead(e);
            Agnode_t* succ = agtail(e);
            if (pred != node && find(predecessors.begin(), predecessors.end(), pred) == predecessors.end()) {
                predecessors.push_back(pred);
            }
            if (succ != node && find(successors.begin(), successors.end(), succ) == successors.end()) {
                successors.push_back(succ);
            }
        }

        // Aggiungi nuovi collegamenti
        for (Agnode_t* pred : predecessors) {
            for (Agnode_t* succ : successors) {
                if (!agedge(g, pred, succ, nullptr, FALSE)) {
                    Agedge_t* new_edge = agedge(g, pred, succ, nullptr, TRUE);
                    if (new_edge) {
                        const char* edge_label = agget(agedge(g, pred, node, nullptr, FALSE), const_cast<char*>("label"));
                        if (edge_label) {
                            agset(new_edge, const_cast<char*>("label"), const_cast<char*>(edge_label));
                        }
                    }
                }
            }
        }
    }

    // Rimuovi i nodi
    for (Agnode_t* node : nodes_to_remove) {
        agdelete(g, node);
    }
}


unordered_map<string, string> read_config(const string& filename) {
    unordered_map<string, string> config;
    ifstream file(filename);
    
    if (!file.is_open()) {
        cerr << "Errore nell'aprire il file: " << filename << endl;
        return config;
    }

    string line;
    while (getline(file, line)) {
        // Ignora le linee vuote o che iniziano con '[' (es. [parameters])
        if (line.empty() || line[0] == '[') {
            continue;
        }

        // Trova la posizione del segno "="
        size_t pos = line.find('=');
        if (pos != string::npos) {
            string key = line.substr(0, pos);
            string value = line.substr(pos + 1);

            // Rimuove eventuali spazi bianchi ai lati delle stringhe
            key.erase(key.find_last_not_of(" \t\n\r\f\v") + 1);
            value.erase(value.find_last_not_of(" \t\n\r\f\v") + 1);

            // Inserisce il parametro nel dizionario
            config[key] = value;
        }
    }

    file.close();
    return config;
}

// Utility function to remove specific characters from a string
string remove_chars(const std::string& str, const std::string& chars_to_remove) {
    std::string result;
    result.reserve(str.size());
    for (char ch : str) {
        if (chars_to_remove.find(ch) == std::string::npos) {
            result += ch;
        }
    }
    return result;
}

// Funzione per stampare la matrice di ID
void print_block_id_matrix(const vector<vector<string>>& block_id_matrix) {
    cout << "Block ID Matrix:\n";
    for (const auto& row : block_id_matrix) {
        for (const auto& id : row) {
            cout << id << " ";
        }
        cout << "\n";
    }
}

void print_block_dict(const unordered_map<std::string, Block>& block_dict) {
    cout << "Block Dictionary:\n";
    for (const auto& pair : block_dict) {
        const Block& block = pair.second;
        print_block(block);
        cout << "-------------------\n";
    }
}

void print_vector(const vector<string>& vec) {
    cout << "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        cout << vec[i];
        if (i != vec.size() - 1) {
            cout << ", ";  // Aggiungi una virgola tra gli elementi
        }
    }
    cout << "]" << endl;
}
