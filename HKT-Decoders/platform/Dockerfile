FROM node:20-alpine

# 同步按钮依赖 git
RUN apk add --no-cache git

WORKDIR /app
COPY package.json ./
RUN npm install --omit=dev --no-audit --no-fund
COPY server.js ./
COPY lib ./lib
COPY web ./web

ENV PORT=8620
EXPOSE 8620
CMD ["node", "server.js"]
