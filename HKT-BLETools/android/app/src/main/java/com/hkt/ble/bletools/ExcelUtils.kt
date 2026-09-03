package com.hkt.ble.bletools

import android.annotation.SuppressLint
import android.os.Environment
import org.apache.poi.ss.usermodel.Row
import org.apache.poi.ss.usermodel.Workbook
import org.apache.poi.xssf.usermodel.XSSFWorkbook
import java.io.File
import java.io.FileInputStream
import java.io.FileOutputStream
import java.io.IOException
import java.text.SimpleDateFormat
import java.util.Date


object ExcelUtil {
    @SuppressLint("SimpleDateFormat")
    fun createOrAppendExcel() {
        // 获取当前日期格式化后的字符串作为文件名
        val sdf = SimpleDateFormat("yyyy-MM-dd")
        val fileName = sdf.format(Date()) + ".xlsx"

        // 判断是否存在SD卡挂载
        if (Environment.getExternalStorageState() != Environment.MEDIA_MOUNTED) {
            return
        }
        val rootDir = Environment.getExternalStorageDirectory()
        val excelFile = File(rootDir, fileName)
        val workbook: Workbook
        try {
            if (excelFile.exists()) {
                // 如果文件存在，读取已有的工作簿
                val fis = FileInputStream(excelFile)
                workbook = XSSFWorkbook(fis)
                fis.close()
            } else {
                // 如果文件不存在，创建新的工作簿
                workbook = XSSFWorkbook()
            }

            // 获取或创建工作表（这里假设只有一个工作表，可根据实际需求调整）
            var sheet = workbook.getSheetAt(0)
            if (sheet == null) {
                sheet = workbook.createSheet("Sheet1")
            }

            // 获取当前行数（用于新增数据到末尾）
            var rowCount = sheet!!.lastRowNum
            if (rowCount == -1) {
                rowCount = 0
            }

            // 创建新行并写入示例数据（这里简单写入两行示例数据，可根据实际需求替换）
            var row: Row = sheet.createRow(rowCount)
            var cell = row.createCell(0)
            cell.setCellValue("数据1")
            cell = row.createCell(1)
            cell.setCellValue("数据2")
            row = sheet.createRow(rowCount + 1)
            cell = row.createCell(0)
            cell.setCellValue("数据3")
            cell = row.createCell(1)
            cell.setCellValue("数据4")

            // 将工作簿写入文件
            val fos = FileOutputStream(excelFile)
            workbook.write(fos)
            fos.close()
            workbook.close()
        } catch (e: IOException) {
            e.printStackTrace()
        }
    }
}