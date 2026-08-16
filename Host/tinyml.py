import numbers
import os
os.environ["CUDA_VISIBLE_DEVICES"] = "-1"

import math
import numpy as np
import tensorflow as tf

import matplotlib.pyplot as plt     # New add 用于打印损失曲线和精度曲线

from tensorflow import keras
layers = keras.layers

# SAMPLES_PER_GESTURE = 120     # New: 每个动作120个采样点, 配合采样频率100Hz
# SAMPLES_PER_GESTURE = 60        # New: 每个动作60个采样点, 配合采样频率50Hz
# PRESAMPLE_COUNT = 65        # 每个动作监测阈值前的采样点数 65, 50Hz采样时为1.3s秒
# POSTSAMPLE_COUNT = 65       # 每个动作监测阈值后的采样点数 65, 50Hz采样时约为1.3s秒
PRESAMPLE_COUNT = 50        # 每个动作监测阈值前的采样点数 50, 50Hz采样时为1.0s秒
POSTSAMPLE_COUNT = 80       # 每个动作监测阈值后的采样点数 80, 50Hz采样时约为1.6s秒
SAMPLES_PER_GESTURE = PRESAMPLE_COUNT + POSTSAMPLE_COUNT        # New: 每个动作总采样点数量

Model = None

def CreateModel():
    # create a NN with 2 layers of 16 neurons
    model = tf.keras.Sequential()
    model.add(layers.Dense(32, activation='relu', input_shape=(6 * SAMPLES_PER_GESTURE,)))      # 输入层默认配置
    model.add(layers.Dense(16, activation='relu'))         # 中间层默认配置
    # model.add(layers.Dense(32, activation='relu'))       # 8输出中间层改32, 验证精度略提高的几率更大（数据打乱后没太大影响）!
    # model.add(layers.Dense(2, activation='softmax'))   # 2输出
    model.add(layers.Dense(4, activation='softmax'))    # 4输出 
    # model.add(layers.Dense(5, activation='softmax'))    # 5输出    
    # model.add(layers.Dense(6, activation='softmax'))    # 6输出
    # model.add(layers.Dense(8, activation='softmax'))    # 8输出

    opt_adam = keras.optimizers.Adam()      # 优化器默认配置
    # opt_adam = keras.optimizers.Adam(lr=0.001, beta_1=0.9, beta_2=0.999, epsilon=None, decay=0.0, amsgrad=False)       # 优化器默认参数，效果同上
    # opt_adam = keras.optimizers.Adam(lr=0.0005, beta_1=0.9, beta_2=0.999, epsilon=None, decay=0.0, amsgrad=False)      #学习率改为0.0005
    model.compile(optimizer=opt_adam, loss='categorical_crossentropy', metrics=['categorical_accuracy'])        # 默认配置   
    model.summary()
    
    return model


def PrepareModel(model):        # 改函数实际未用，作用暂时不明
    SAMPLES = 100      # 每个动作100次采样，PrepareModel通常不调用，此处不改也可以
    #SAMPLES = 200       # 每个动作200次采样
    np.random.seed(1337)
    
    x_values = np.random.uniform(low=0, high=2 * math.pi, size=(SAMPLES, 6 * SAMPLES_PER_GESTURE))
    # shuffle and add noise
    np.random.shuffle(x_values)
    # y_values = np.random.uniform(low=0, high=1, size=(SAMPLES, 2))     # 2输出，PrepareModel通常不调用，此处不改也可以
    y_values = np.random.uniform(low=0, high=1, size=(SAMPLES, 4))      # 4输出
    # y_values = np.random.uniform(low=0, high=1, size=(SAMPLES, 5))      # 5输出
    # y_values = np.random.uniform(low=0, high=1, size=(SAMPLES, 6))      # 6输出
    # y_values = np.random.uniform(low=0, high=1, size=(SAMPLES, 8))      # 8输出
    #y_values = np.random.randn(*y_values.shape)

    return x_values, y_values
    

def TrainModel(model, x, y):
    SampleCount = len(x)
    # New add for shuffle: 打乱数据集顺序，即数据与标签同步打乱‌：生成相同随机索引，保持数据与标签的对应关系‌。
    # print(x)
    # print(y)
    index = np.random.permutation(x.shape[0])    # 生成与整个数据集长度一样的索引index，具体如果共有8个动作，每个动作100样本，则共800样本，则生成0~799范围内随机排列的index
    x = x[index]    # 按index索引重新同步排列x和y
    y = y[index]
    # print(index)
    # print(x)
    # print(y)

    # split into train, validation, test
    # TRAIN_SPLIT =  int(0.8 * SampleCount)     # 默认训练样本与验证样本比列为8:2
    TRAIN_SPLIT =  int(0.8 * SampleCount)       # New 5、6/8输出时, 需按85%/90%训练样本(每个动作按100样本计), 否则训练结果很好验证结果很差!
                                                 # 注：加入上面打乱数据集顺序的代码后基本无此问题！
    x_train, x_validate = np.split(x, [TRAIN_SPLIT, ])
    y_train, y_validate = np.split(y, [TRAIN_SPLIT, ])

    # model.fit(x_train, y_train, epochs=200, batch_size=16, validation_data=(x_validate, y_validate))    # 默认训练配置
    
    history = model.fit(x_train, y_train, epochs=30, batch_size=16, validation_data=(x_validate, y_validate))   # New change 用于打印损失曲线和精度曲线
    return history      # New add 用于打印损失曲线和精度曲线


def ConvertModel(model):
    converter = tf.lite.TFLiteConverter.from_keras_model(model)
    tflite_model = converter.convert()

    return tflite_model

def SaveModel(model):
    #with open("model.tflite", "wb") as f:
        #f.write(model)
    
    base_path = os.path.dirname(__file__)
    with open(base_path + "/" + "model.tflite", 'wb') as f:
        f.write(model)




# Function: Convert some hex value into an array for C programming
def Hex2H(model, h_model_name):   
    c_str = ''
    model_len = len(model)

    # Create header guard
    c_str += '#ifndef ' + h_model_name.upper() + '_H\n'
    c_str += '#define ' + h_model_name.upper() + '_H\n'

    # Add array length at top of file
    c_str += '\nconst unsigned int ' + h_model_name + '_len = ' + str(model_len) + ';\n'

    # Declare C variable
    c_str += 'const unsigned char ' + h_model_name + '[] = {'
    hex_array = []
    for i, val in enumerate(model) :
        # Construct string from hex
        hex_str = format(val, '#04x')

        # Add formatting so each line stays within 80 characters
        if (i + 1) < model_len:
          hex_str += ','
        if (i + 1) % 12 == 0:
          hex_str += '\n '
        hex_array.append(hex_str)

    # Add closing brace
    c_str += '\n ' + format(' '.join(hex_array)) + '\n};\n\n'

    # Close out header guard
    c_str += '#endif //' + h_model_name.upper() + '_H\n'
        
    # Write TFLite model to a C source (or header) file
    base_path = os.path.dirname(__file__)
    with open(base_path + "/" + h_model_name + '.h', 'w') as file:
        file.write(c_str)   
        file.flush()


def ReadDataFile(file, v):
    size = SAMPLES_PER_GESTURE * 6

    dataX = np.empty([0, size])
    # dataY = np.empty([0,])
    # dataY = np.empty([0, 2])   # 2输出
    dataY = np.empty([0, 4])    # 4输出
    # dataY = np.empty([0, 5])    # 5输出
    # dataY = np.empty([0, 6])    # 6输出
    # dataY = np.empty([0, 8])    # 8输出

    base_path = os.path.dirname(__file__)
    file = open(base_path + "/data/" + file, "r")

    data = []
    for line in file.readlines():
        items = line.split()
        values = [int(v) / 32768 for v in items ]       # 此处v为传回的六轴数据，范围为[-32768,+32768]；values为归一化的六轴数据，范围为[-1,+1]
                                                        # 实际三轴加速度和三轴角速度应考虑ESP32端MPU6050的量程设置，即
                                                        # 如加速度量程设为+-8g，则此处实际加速度值为values*8g; 如角速度量程设为+-1000deg，则此处实际角速度值为values*1000deg
        data += values        

    count = len(data)

    for i in range(0, count, size):
        tmp = np.array(data[i: i + size])
        if len(tmp) == size:
            tmp = np.expand_dims(tmp, axis=0)

            dataX = np.concatenate((dataX, tmp), axis=0)
            # dataY = np.append(dataY, v)

            # 4输出
            if v == 0:
                dataY = np.concatenate((dataY, [[1, 0, 0, 0]]))
            if v == 1:
                dataY = np.concatenate((dataY, [[0, 1, 0, 0]]))
            if v == 2:
                dataY = np.concatenate((dataY, [[0, 0, 1, 0]]))
            if v == 3:
                dataY = np.concatenate((dataY, [[0, 0, 0, 1]]))

            # 5输出
            # if v == 0:
            #     dataY = np.concatenate((dataY, [[1, 0, 0, 0, 0]]))
            # if v == 1:
            #     dataY = np.concatenate((dataY, [[0, 1, 0, 0, 0]]))
            # if v == 2:
            #     dataY = np.concatenate((dataY, [[0, 0, 1, 0, 0]]))
            # if v == 3:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 1, 0]]))
            # if v == 4:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 1]]))

            # 6输出
            # if v == 0:
            #     dataY = np.concatenate((dataY, [[1, 0, 0, 0, 0, 0]]))
            # if v == 1:
            #     dataY = np.concatenate((dataY, [[0, 1, 0, 0, 0, 0]]))
            # if v == 2:
            #     dataY = np.concatenate((dataY, [[0, 0, 1, 0, 0, 0]]))
            # if v == 3:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 1, 0, 0]]))
            # if v == 4:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 1, 0]]))
            # if v == 5:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 0, 1]]))
    
            # 8输出
            # if v == 0:
            #     dataY = np.concatenate((dataY, [[1, 0, 0, 0, 0, 0, 0, 0]]))
            # if v == 1:
            #     dataY = np.concatenate((dataY, [[0, 1, 0, 0, 0, 0, 0, 0]]))
            # if v == 2:
            #     dataY = np.concatenate((dataY, [[0, 0, 1, 0, 0, 0, 0, 0]]))
            # if v == 3:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 1, 0, 0, 0, 0]]))
            # if v == 4:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 1, 0, 0, 0]]))
            # if v == 5:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 0, 1, 0, 0]]))
            # if v == 6:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 0, 0, 1, 0]]))
            # if v == 7:
            #     dataY = np.concatenate((dataY, [[0, 0, 0, 0, 0, 0, 0, 1]]))

    return dataX, dataY


def ReadTrainingData():
    # 4输出
    sitdown_x, sitdown_y = ReadDataFile("sitdown.csv", 3)
    jog_x, jog_y = ReadDataFile("jog.csv", 2)
    jump_x, jump_y = ReadDataFile("jump.csv", 1)
    fall_x, fall_y = ReadDataFile("fall.csv", 0) 
    # 4输出
    # walk_x, walk_y = ReadDataFile("walk.csv", 3)
    # jog_x, jog_y = ReadDataFile("jog.csv", 2)
    # jump_x, jump_y = ReadDataFile("jump.csv", 1)
    # fall_x, fall_y = ReadDataFile("fall.csv", 0) 
    # 5输出    
    # sitdown_x, sitdown_y = ReadDataFile("sitdown.csv", 4)
    # walk_x, walk_y = ReadDataFile("walk.csv", 3)
    # jog_x, jog_y = ReadDataFile("jog.csv", 2)
    # jump_x, jump_y = ReadDataFile("jump.csv", 1)
    # fall_x, fall_y = ReadDataFile("fall.csv", 0)    
    # 6输出
    # downstairs_x, downstairs_y = ReadDataFile("downstairs.csv", 5)
    # sitdown_x, sitdown_y = ReadDataFile("sitdown.csv", 4)
    # walk_x, walk_y = ReadDataFile("walk.csv", 3)
    # jog_x, jog_y = ReadDataFile("jog.csv", 2)
    # jump_x, jump_y = ReadDataFile("jump.csv", 1)
    # fall_x, fall_y = ReadDataFile("fall.csv", 0)
    # 8输出
    # upstairs_x, upstairs_y = ReadDataFile("upstairs.csv", 7)
    # downstairs_x, downstairs_y = ReadDataFile("downstairs.csv", 6)
    # situp_x, situp_y = ReadDataFile("situp.csv", 5)
    # sitdown_x, sitdown_y = ReadDataFile("sitdown.csv", 4)
    # walk_x, walk_y = ReadDataFile("walk.csv", 3)
    # jog_x, jog_y = ReadDataFile("jog.csv", 2)
    # jump_x, jump_y = ReadDataFile("jump.csv", 1)
    # fall_x, fall_y = ReadDataFile("fall.csv", 0)

    # 4输出
    dataX = np.concatenate((sitdown_x, jog_x, jump_x, fall_x), axis=0)
    dataY = np.concatenate((sitdown_y, jog_y, jump_y, fall_y), axis=0)
    # 4输出
    # dataX = np.concatenate((walk_x, jog_x, jump_x, fall_x), axis=0)
    # dataY = np.concatenate((walk_y, jog_y, jump_y, fall_y), axis=0)
    # 5输出
    # dataX = np.concatenate((sitdown_x, walk_x, jog_x, jump_x, fall_x), axis=0)
    # dataY = np.concatenate((sitdown_y, walk_y, jog_y, jump_y, fall_y), axis=0)
    # 6输出
    # dataX = np.concatenate((downstairs_x, sitdown_x, walk_x, jog_x, jump_x, fall_x), axis=0)
    # dataY = np.concatenate((downstairs_y, sitdown_y, walk_y, jog_y, jump_y, fall_y), axis=0)
    # 8输出
    # dataX = np.concatenate((upstairs_x, downstairs_x, situp_x, sitdown_x, walk_x, jog_x, jump_x, fall_x), axis=0)
    # dataY = np.concatenate((upstairs_y, downstairs_y, situp_y, sitdown_y, walk_y, jog_y, jump_y, fall_y), axis=0)
    
    return dataX, dataY

if __name__ == '__main__':
    Model = CreateModel()
    # PrepareModel(Model)     #原代码此句注释, 加入噪声?
    
    x, y = ReadTrainingData()
    # TrainModel(Model, x, y)
    history = TrainModel(Model, x, y)   # New change  引入history便于后面打印损失曲线和精度曲线  

    print("==== Convert Model")
    tfModel = ConvertModel(Model)

    print("==== Save Model")
    SaveModel(tfModel)

    print("==== Make Header File")
    Hex2H(tfModel, "model")

    print("==== Model Build Finished")

    # New add 打印损失曲线和精度曲线
    # print(history.history.keys())     # New add

    # ==== 修正绘图部分 ====
    plt.figure(figsize=(12, 5))

    # 1. 绘制损失曲线
    plt.subplot(1, 2, 1)
    loss = history.history['loss']
    val_loss = history.history['val_loss']
    epochs = range(1, len(loss) + 1)
    plt.plot(epochs, loss, 'bo-', label='Training loss')
    plt.plot(epochs, val_loss, 'r^-', label='Validation loss')
    plt.title('Training and Validation Loss')
    plt.xlabel('Epochs')
    plt.ylabel('Loss')
    plt.legend()
    plt.grid(True)

    # 2. 绘制精度曲线
    plt.subplot(1, 2, 2)
    acc = history.history['categorical_accuracy']
    val_acc = history.history['val_categorical_accuracy']
    plt.plot(epochs, acc, 'bo-', label='Training accuracy')
    plt.plot(epochs, val_acc, 'r^-', label='Validation accuracy')
    plt.title('Training and Validation Accuracy')
    plt.xlabel('Epochs')
    plt.ylabel('Accuracy')
    plt.legend()
    plt.grid(True)

    plt.tight_layout()
    plt.show()