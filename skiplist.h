/* ************************************************************************
> File Name:     skiplist.h
> Author:        程序员Carl
> 微信公众号:    代码随想录
> Created Time:  Sun Dec  2 19:04:26 2018
> Description:   
 ************************************************************************/
//TODO: 明早起来在云服务器编译，做压测，然后修改TODO
#include <iostream> 
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx;     // mutex for critical section
std::string delimiter = ":";

//Class template to implement node
//节点的定义，模板化，K，V
template<typename K, typename V> 
class Node {

public:
    
    Node() {} 

    Node(K k, V v, int); 

    ~Node();

    K get_key() const;

    V get_value() const;

    void set_value(V);
    
    // Linear array to hold pointers to next node of different level
    //存的是指向Node的指针，是一个指针数组，后面使用了下标访问
    Node<K, V> **forward;  //指向下一层的对应节点

    int node_level;

private:
    K key;
    V value;
};
// 构造函数
template<typename K, typename V> 
Node<K, V>::Node(const K k, const V v, int level) {
    this->key = k;  //设置key
    this->value = v;  //设置value
    this->node_level = level;  //设置当前节点的层级

    // level + 1, because array index is from 0 - level
    this->forward = new Node<K, V>*[level+1];  //forward是一个指针数组，初始化长度为当前节点的层级+1，它存储的是每个节点在第i个层级的直接后继节点
    
	// Fill forward array with 0(NULL) 
    memset(this->forward, 0, sizeof(Node<K, V>*)*(level+1));  //清空forward所占用的内存，避免脏读
};

template<typename K, typename V> 
Node<K, V>::~Node() {
    delete []forward;  //析构函数，释放forward指针所指向内存
};

template<typename K, typename V> 
K Node<K, V>::get_key() const {
    return key;  //取键
};

template<typename K, typename V> 
V Node<K, V>::get_value() const {
    return value;  //取值
};

template<typename K, typename V> 
void Node<K, V>::set_value(V value) {
    this->value=value;  //设置值
};

// Class template for Skip list
template <typename K, typename V> 
class SkipList {  //跳表类模板

public: 
    SkipList(int);
    ~SkipList();
    int get_random_level();
    Node<K, V>* create_node(K, V, int);  //创建节点
    int insert_element(K, V);  //插入元素
    void display_list();  //跳表可视化
    bool search_element(K);  //元素查找
    void delete_element(K);  //元素删除
    void dump_file();  //输出
    void load_file();  //加载
    int size();  //返回跳表大小

private:
    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);  //这个方法是干啥的
    bool is_valid_string(const std::string& str);  //这个方法是干啥的

private:    
    // Maximum level of the skip list 
    int _max_level;

    // current level of skip list 
    int _skip_list_level;

    // pointer to header node 
    Node<K, V> *_header;

    // file operator
    std::ofstream _file_writer;
    std::ifstream _file_reader;

    // skiplist current element count
    int _element_count;
};

// create new node 
template<typename K, typename V>
Node<K, V>* SkipList<K, V>::create_node(const K k, const V v, int level) {
    Node<K, V> *n = new Node<K, V>(k, v, level);  //新建节点，返回指向这个节点的指针
    return n;
}

// Insert given key and value in skip list 
// return 1 means element exists  //跳表中不含重复元素
// return 0 means insert successfully
/* 
                           +------------+
                           |  insert 50 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |                      insert +----+
level 3         1+-------->10+---------------> | 50 |          70       100
                                               |    |
                                               |    |
level 2         1          10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 1         1    4     10         30       | 50 |          70       100
                                               |    |
                                               |    |
level 0         1    4   9 10         30   40  | 50 |  60      70       100
                                               +----+

*/
template<typename K, typename V>
//向跳表中插入元素，包括：要找到插在哪里，是不是要提高层级，插入之后所有层级链表的调整
int SkipList<K, V>::insert_element(const K key, const V value) {
    
    mtx.lock();  //线程锁
    Node<K, V> *current = this->_header;  //curr指向跳表的链表头

    // create update array and initialize it 
    // update is array which put node that the node->forward[i] should be operated later
    Node<K, V> *update[_max_level+1];  //新建的update是数组，存的是指向Node<k,v>的指针，_max_level是跳表的最高层，
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));  //清空update数组中的内容

    // start form highest level of skip list 
    for(int i = _skip_list_level; i >= 0; i--) {  //从最高级往下一层层遍历
        //如果当前节点的后驱节点存在且key小于待插入key，要在当前层向后走，否则的话需要下沉
        while(current->forward[i] != NULL && current->forward[i]->get_key() < key) { 
            current = current->forward[i]; 
        }
        update[i] = current;  //记录第i层需要update的节点位置，方便一会儿插入数据后更新上级索引
    }

    // reached level 0 and forward pointer to right node, which is desired to insert key.
    current = current->forward[0];

    // if current node have key equal to searched key, we get it，刚好hit the point
    if (current != NULL && current->get_key() == key) {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return 1;
    }

    // if current is NULL that means we have reached to end of the level 
    // if current's key is not equal to key that means we have to insert node between update[0] and current node 
    //前面把current向后移动了一个节点，实际的插入位置应当是update[0]和current之间的位置
    if (current == NULL || current->get_key() != key ) {
        
        // Generate a random level for node，随机提升为几级索引，get_random_level()是一个随机数发生器
        //get_random_Level保证了返回的索引级别不会超过设置的最高索引级别
        int random_level = get_random_level();

        // If random level is greater thar skip list's current level, initialize update value with pointer to header
        if (random_level > _skip_list_level) {
            //对于超出的这一部分level，待更新的节点其实就是dummy head
            for (int i = _skip_list_level+1; i < random_level+1; i++) {
                update[i] = _header;
            }
            //更新跳表的高度
            _skip_list_level = random_level;
        }

        // create new node with random level generated 
        //new一个新节点
        Node<K, V>* inserted_node = create_node(key, value, random_level);
        
        // insert node 
        //逐层插入这个节点
        for (int i = 0; i <= random_level; i++) {
            inserted_node->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = inserted_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        _element_count ++;
    }
    mtx.unlock();
    return 0;
}

// Display skip list 
template<typename K, typename V> 
void SkipList<K, V>::display_list() {

    std::cout << "\n*****Skip List*****"<<"\n"; 
    for (int i = 0; i <= _skip_list_level; i++) {
        Node<K, V> *node = this->_header->forward[i];   //取出当前层的第一个节点
        std::cout << "Level " << i << ": ";  //输出本层的标识符
        while (node != NULL) {  //依次打印本层节点
            std::cout << node->get_key() << ":" << node->get_value() << ";";
            node = node->forward[i];
        }
        std::cout << std::endl;
    }
}

// Dump data in memory to file 
template<typename K, typename V> 
void SkipList<K, V>::dump_file() {

    std::cout << "dump_file-----------------" << std::endl;
    _file_writer.open(STORE_FILE);  //_file_writer: std::ofstream
    //注意，只存储0级节点，不存储完整的条表结构
    Node<K, V> *node = this->_header->forward[0]; //也是同样拿到第一个节点，不保存dummy head

    while (node != NULL) {
        _file_writer << node->get_key() << ":" << node->get_value() << "\n";  //每行一个键值对，读取的时候也是一样的格式
        std::cout << node->get_key() << ":" << node->get_value() << ";\n";
        node = node->forward[0];  
    }

    _file_writer.flush();
    _file_writer.close();
    return ;
}

// Load data from disk
template<typename K, typename V> 
void SkipList<K, V>::load_file() {

    _file_reader.open(STORE_FILE);  //_file_reader: std::ifstream
    std::cout << "load_file-----------------" << std::endl;
    std::string line;
    std::string* key = new std::string();  //从文件读取，读到的是字符串
    std::string* value = new std::string();
    while (getline(_file_reader, line)) {  //一行一行地读取，按照输出的格式，每一行是一个键值对
        //TODO: 如果这里要把key和Value扩展为自定义类型，需要考虑如何从文件读取，怎么规定key和value的类型？怎么保证读取？
        get_key_value_from_string(line, key, value);  //从读入的行获取key和value，把key和value转化为对应的键值对类型
        if (key->empty() || value->empty()) {
            continue;
        }
        insert_element(*key, *value);  //重建跳表的索引结构，这里的索引肯定就和写入的时候不一样了，不过没关系，0层的数据都没有漏掉
        std::cout << "key:" << *key << "value:" << *value << std::endl;
    }
    _file_reader.close();
}

// Get current SkipList size 
template<typename K, typename V> 
int SkipList<K, V>::size() { 
    return _element_count;
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value) {

    if(!is_valid_string(str)) {
        return;
    }
    *key = str.substr(0, str.find(delimiter));  //按分隔符获取key和value
    *value = str.substr(str.find(delimiter)+1, str.length());
}

template<typename K, typename V>
bool SkipList<K, V>::is_valid_string(const std::string& str) {

    if (str.empty()) {
        return false;
    }
    if (str.find(delimiter) == std::string::npos) {  //std::string::npos用于表示一个不存在的值，也即这个分隔符不存在
        return false;
    }
    return true;
}

// Delete element from skip list 
template<typename K, typename V> 
//从跳表中删除元素，包括：找到元素在哪里，把其在更高层级上的索引都删掉，并连接前后节点使得表结构不被破坏
void SkipList<K, V>::delete_element(K key) {
    //前面的步骤和插入是一样的，因为都需要在0层找到这个元素在哪里，才能进行后续的插删操作
    mtx.lock();
    Node<K, V> *current = this->_header; 
    Node<K, V> *update[_max_level+1];
    memset(update, 0, sizeof(Node<K, V>*)*(_max_level+1));

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] !=NULL && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
        update[i] = current;
    }

    current = current->forward[0];
    //到这里，update[i]存储的是待删除结点在各层的（理论）前驱节点，current是待删除结点在0层的位置
    if (current != NULL && current->get_key() == key) {  //如果不满足这个条件，说明待删除结点并不在跳表里，TODO: 是否要加一个提示呢？
       
        // start for lowest level and delete the current node of each level
        for (int i = 0; i <= _skip_list_level; i++) {

            // if at level i, next node is not target node, break the loop.
            if (update[i]->forward[i] != current) 
                break;

            update[i]->forward[i] = current->forward[i];  //被删除节点没有释放内存空间，应当在元素最后被删除的时候释放掉，TODO: 用shared_ptr去管理是不是更合适？
        }

        // Remove levels which have no elements
        //有的层可能在删除索引之后，就没有实际的节点了
        while (_skip_list_level > 0 && _header->forward[_skip_list_level] == 0) {  //TODO: 为什么不是==NULL？
            _skip_list_level --; 
        }

        std::cout << "Successfully deleted key "<< key << std::endl;
        _element_count --;
    }
    mtx.unlock();
    return;
}

// Search for element in skip list 
/*
                           +------------+
                           |  select 60 |
                           +------------+
level 4     +-->1+                                                      100
                 |
                 |
level 3         1+-------->10+------------------>50+           70       100
                                                   |
                                                   |
level 2         1          10         30         50|           70       100
                                                   |
                                                   |
level 1         1    4     10         30         50|           70       100
                                                   |
                                                   |
level 0         1    4   9 10         30   40    50+-->60      70       100
*/
template<typename K, typename V> 
bool SkipList<K, V>::search_element(K key) {  //和插入是一样的套路

    std::cout << "search_element-----------------" << std::endl;
    Node<K, V> *current = _header;

    // start from highest level of skip list
    for (int i = _skip_list_level; i >= 0; i--) {
        while (current->forward[i] && current->forward[i]->get_key() < key) {
            current = current->forward[i];
        }
    }

    //reached level 0 and advance pointer to right node, which we search
    current = current->forward[0];

    // if current node have key equal to searched key, we get it
    if (current and current->get_key() == key) {
        std::cout << "Found key: " << key << ", value: " << current->get_value() << std::endl;
        return true;
    }

    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

// construct skip list
template<typename K, typename V> 
SkipList<K, V>::SkipList(int max_level) {  //构造函数

    this->_max_level = max_level;
    this->_skip_list_level = 0;  //跳表当前高度
    this->_element_count = 0;  //跳表当前元素数

    // create header node and initialize key and value to null，TODO: 为什么不给kv初始化，这是合理的吗，不是说最好都初始化吗
    K k;
    V v;
    this->_header = new Node<K, V>(k, v, _max_level);
};

template<typename K, typename V> 
SkipList<K, V>::~SkipList() {  //析构函数

    if (_file_writer.is_open()) {
        _file_writer.close();
    }
    if (_file_reader.is_open()) {
        _file_reader.close();
    }
    delete _header;  //只释放了_header，没有释放后续的空间，TODO: 完善析构函数
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level(){  //随机数发生器，生成的分布是：

    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < _max_level) ? k : _max_level;
    return k;
};
// vim: et tw=100 ts=4 sw=4 cc=120
