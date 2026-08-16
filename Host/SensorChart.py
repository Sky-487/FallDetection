

from OpenGL import GLUT, GLU, GL

# PRESAMPLE_COUNT = 65        # 每个动作监测阈值前的采样点数 65, 50Hz采样时为1.3s秒
# POSTSAMPLE_COUNT = 65       # 每个动作监测阈值后的采样点数 65, 50Hz采样时约为1.3s秒
PRESAMPLE_COUNT = 50        # 每个动作监测阈值前的采样点数 50, 50Hz采样时为1.0s秒
POSTSAMPLE_COUNT = 80       # 每个动作监测阈值后的采样点数 80, 50Hz采样时约为1.6s秒
SAMPLES_PER_GESTURE = PRESAMPLE_COUNT + POSTSAMPLE_COUNT        # Hans: 每个动作总采样点数量

AxisColor = [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
BarScale = [0.00005, 0.00005, 0.00005, 0.00005, 0.00005, 0.00005, 0.0015, 0.0015, 0.0015]

class SensorChart:
    def __init__(self):
        GLUT.glutInit()
        GLUT.glutInitDisplayMode(GLUT.GLUT_SINGLE | GLUT.GLUT_RGBA)
        GLUT.glutInitWindowSize(600, 600)
        # GLUT.glutInitWindowSize(800, 800)       # Hans
        GLUT.glutCreateWindow("3D")
        GL.glClearColor(1.0, 1.0, 1.0, 1.0);  # Hans add 设置背景颜色为白色, 如为默认黑色注销此句


    def Run(self, proc):
        GLUT.glutDisplayFunc(self.Draw)
        GLUT.glutIdleFunc(proc)

        GLUT.glutMainLoop()


    def Draw(self, data=None, data_his=None):
        GL.glClear(GL.GL_COLOR_BUFFER_BIT)
        GL.glBegin(GL.GL_LINES)

        #YScale = lambda y: y / 5 * 0.00001 
        #Hans: 原代码中 SMOTH_COUNT = 5, 即每次采样快速读5遍相加, 相当于扩大5倍. 改为SMOTH_COUNT = 1后, 此处相应改动
        # YScale = lambda y: y * 0.00001
        YScale = lambda y: y * 0.00001 * 1.5      # Hans：对于+-8g, +-1000deg/s通常情况下幅度太小，需要放大1.5被以便观察
        # YScale = lambda y: y * 0.00001 * 3      # Hans：对于+-16g, +-2000deg/s通常情况下幅度太小，需要放大3被以便观察

        # Draw Bars
        if data != None:
            for i in range(len(data)):
                x = i * 0.1 - 0.9
                y = 0.75 + YScale(data[i])
                c = AxisColor[i % 3]

                GL.glColor3f(c[0], c[1], c[2])
                GL.glVertex3f(x, 0.75, 0)
                GL.glVertex3f(x, y, 0)

        # Draw Curves
        if data_his != None and len(data_his) > 0:
            size = len(data_his)
            for i in range(6):
                c = AxisColor[i % 3]
                # y0 = 0.1 if i > 2 else -0.6
                y0 = 0.2 if i > 2 else -0.6     # Hans
                GL.glColor3f(c[0], c[1], c[2])
                    
                px = -1
                py = y0 + YScale(data_his[0][i])   
                for k in range(size - 1):
                    # x = -1 + (k + 1) *  2 / 120       # Hans: 每个动作120个采样点, 配合采样频率100Hz
                    # x = -1 + (k + 1) *  2 / 60       # Hans: 每个动作60个采样点, 配合采样频率50Hz
                    x = -1 + (k + 1) *  2 / SAMPLES_PER_GESTURE          # Hans: 每个动作总采样点数量SAMPLES_PER_GESTURE
                    y =  y0 + YScale(data_his[k + 1][i])
                    GL.glVertex3f(px, py, 0)
                    GL.glVertex3f(x, y, 0)
                    px = x
                    py = y


        GL.glEnd()
        GL.glFlush()